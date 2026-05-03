//go:build ignore

package main

import (
	"bytes"
	"flag"
	"fmt"
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"sort"
	"strings"
	"sync"
)

var dumpSkipParts = map[string]bool{
	".cache":             true,
	".codex":             true,
	".espressif":         true,
	".git":               true,
	".rustup":            true,
	"build":              true,
	"dump":               true,
	"esp-idf":            true,
	"managed_components": true,
}

var dumpAllowedSuffixes = map[string]bool{
	".c":     true,
	".cc":    true,
	".cpp":   true,
	".cxx":   true,
	".h":     true,
	".hh":    true,
	".hpp":   true,
	".hxx":   true,
	".cmake": true,
	".txt":   true,
}

func main() {
	root, err := repoRoot()
	if err != nil {
		fatal(err)
	}
	out := filepath.Join(root, "dump", "files")
	flags := flag.NewFlagSet(os.Args[0], flag.ExitOnError)
	flags.StringVar(&out, "o", out, "output directory")
	flags.StringVar(&out, "output", out, "output directory")
	if err := flags.Parse(os.Args[1:]); err != nil {
		fatal(err)
	}
	if flags.NArg() > 0 {
		out = flags.Arg(0)
	}
	if out != "" {
		out, err = filepath.Abs(out)
		if err != nil {
			fatal(err)
		}
	}
	if out == root || !inside(out, root) {
		fatal(fmt.Errorf("dump output must stay inside this repository"))
	}

	files, err := trackedAndLocal(root)
	if err != nil {
		files, err = walkedFiles(root)
	}
	if err != nil {
		fatal(err)
	}
	sort.Strings(files)

	if err := os.RemoveAll(out); err != nil {
		fatal(err)
	}
	if err := os.MkdirAll(out, 0o755); err != nil {
		fatal(err)
	}

	type item struct {
		name string
		rel  string
	}
	var copied []item
	used := map[string]bool{}
	type job struct {
		src  string
		dst  string
		name string
		rel  string
	}
	jobs := make(chan job)
	errs := make(chan error, 1)
	var wg sync.WaitGroup
	workers := 8
	for i := 0; i < workers; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for job := range jobs {
				if err := copyFile(job.src, job.dst); err != nil {
					select {
					case errs <- err:
					default:
					}
				}
			}
		}()
	}

	for _, rel := range files {
		if !wanted(rel) {
			continue
		}
		src := filepath.Join(root, filepath.FromSlash(rel))
		info, err := os.Stat(src)
		if err != nil || info.IsDir() {
			continue
		}
		name := flatName(rel, used)
		copied = append(copied, item{name: name, rel: rel})
		jobs <- job{src: src, dst: filepath.Join(out, name), name: name, rel: rel}
	}
	close(jobs)
	wg.Wait()
	select {
	case err := <-errs:
		fatal(err)
	default:
	}

	var manifest bytes.Buffer
	for _, item := range copied {
		fmt.Fprintf(&manifest, "%s\t%s\n", item.name, item.rel)
	}
	if err := os.WriteFile(filepath.Join(out, "MANIFEST.txt"), manifest.Bytes(), 0o644); err != nil {
		fatal(err)
	}
	relOut, _ := filepath.Rel(root, out)
	fmt.Printf("dumped %d files into %s\n", len(copied), filepath.ToSlash(relOut))
}

func repoRoot() (string, error) {
	exe, err := os.Getwd()
	if err != nil {
		return "", err
	}
	for {
		if _, err := os.Stat(filepath.Join(exe, ".git")); err == nil {
			return exe, nil
		}
		next := filepath.Dir(exe)
		if next == exe {
			return "", fmt.Errorf("not inside a git repository")
		}
		exe = next
	}
}

func trackedAndLocal(root string) ([]string, error) {
	cmd := exec.Command("git", "ls-files", "-co", "--exclude-standard")
	cmd.Dir = root
	out, err := cmd.Output()
	if err != nil {
		return nil, err
	}
	lines := strings.Split(strings.TrimSpace(string(out)), "\n")
	if len(lines) == 1 && lines[0] == "" {
		return nil, nil
	}
	seen := map[string]bool{}
	var files []string
	for _, line := range lines {
		rel := filepath.ToSlash(strings.TrimSpace(line))
		if rel != "" && !seen[rel] {
			files = append(files, rel)
			seen[rel] = true
		}
	}
	return files, nil
}

func walkedFiles(root string) ([]string, error) {
	var files []string
	err := filepath.WalkDir(root, func(path string, entry os.DirEntry, err error) error {
		if err != nil {
			return err
		}
		rel, err := filepath.Rel(root, path)
		if err != nil || rel == "." {
			return err
		}
		rel = filepath.ToSlash(rel)
		if entry.IsDir() && skipped(rel) {
			return filepath.SkipDir
		}
		if !entry.IsDir() {
			files = append(files, rel)
		}
		return nil
	})
	return files, err
}

func skipped(rel string) bool {
	for _, part := range strings.Split(rel, "/") {
		if dumpSkipParts[part] || strings.HasPrefix(part, "build-") {
			return true
		}
	}
	return false
}

func wanted(rel string) bool {
	name := filepath.Base(rel)
	if skipped(rel) || strings.HasPrefix(name, "sdkconfig") {
		return false
	}
	lower := strings.ToLower(name)
	if lower == "readme.md" || name == "CMakeLists.txt" {
		return true
	}
	if strings.HasPrefix(name, "partitions") && filepath.Ext(name) == ".csv" {
		return true
	}
	return dumpAllowedSuffixes[filepath.Ext(name)]
}

func flatName(rel string, used map[string]bool) string {
	candidate := strings.ReplaceAll(rel, "/", "__")
	if !used[candidate] {
		used[candidate] = true
		return candidate
	}
	ext := filepath.Ext(candidate)
	stem := strings.TrimSuffix(candidate, ext)
	for i := 2; ; i++ {
		next := fmt.Sprintf("%s__%d%s", stem, i, ext)
		if !used[next] {
			used[next] = true
			return next
		}
	}
}

func copyFile(src, dst string) error {
	in, err := os.Open(src)
	if err != nil {
		return err
	}
	defer in.Close()
	out, err := os.Create(dst)
	if err != nil {
		return err
	}
	if _, err := io.Copy(out, in); err != nil {
		out.Close()
		return err
	}
	return out.Close()
}

func inside(path, root string) bool {
	rel, err := filepath.Rel(root, path)
	return err == nil && rel != "." && !strings.HasPrefix(rel, "..")
}

func fatal(err error) {
	fmt.Fprintln(os.Stderr, err)
	os.Exit(1)
}
