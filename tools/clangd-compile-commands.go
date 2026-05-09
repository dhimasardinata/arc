//go:build ignore

package main

import (
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"sort"
	"strings"
)

type commandEntry struct {
	Directory string   `json:"directory,omitempty"`
	Command   string   `json:"command,omitempty"`
	Arguments []string `json:"arguments,omitempty"`
	File      string   `json:"file"`
	Output    string   `json:"output,omitempty"`
	extra     map[string]any
}

func (e *commandEntry) UnmarshalJSON(data []byte) error {
	type alias commandEntry
	var raw map[string]any
	if err := json.Unmarshal(data, &raw); err != nil {
		return err
	}
	var out alias
	if err := json.Unmarshal(data, &out); err != nil {
		return err
	}
	*e = commandEntry(out)
	e.extra = raw
	return nil
}

func (e commandEntry) MarshalJSON() ([]byte, error) {
	m := map[string]any{}
	for k, v := range e.extra {
		m[k] = v
	}
	if e.Directory != "" {
		m["directory"] = e.Directory
	}
	if len(e.Arguments) > 0 {
		m["arguments"] = e.Arguments
		delete(m, "command")
	} else if e.Command != "" {
		m["command"] = e.Command
	}
	m["file"] = e.File
	if e.Output != "" {
		m["output"] = e.Output
	}
	return json.Marshal(m)
}

func main() {
	root, err := repoRoot()
	if err != nil {
		fatal(err)
	}

	var output string
	var noArcHeaders bool
	var validateArcHeaders bool
	var validateTimeout float64
	var validateJobs int
	flags := flag.NewFlagSet(os.Args[0], flag.ExitOnError)
	flags.StringVar(&output, "o", filepath.Join(root, "compile_commands.json"), "output compile_commands.json path")
	flags.StringVar(&output, "output", filepath.Join(root, "compile_commands.json"), "output compile_commands.json path")
	flags.BoolVar(&noArcHeaders, "no-arc-headers", false, "do not add Arc header commands")
	flags.BoolVar(&validateArcHeaders, "validate-arc-headers", false, "delegate strict header validation to Python compatibility path")
	flags.Float64Var(&validateTimeout, "validate-timeout", 120, "per-header validation timeout")
	flags.IntVar(&validateJobs, "validate-jobs", 0, "parallel compiler jobs for header validation")
	if err := flags.Parse(os.Args[1:]); err != nil {
		fatal(err)
	}

	if validateArcHeaders {
		args := []string{filepath.Join(root, "tools", "clangd-compile-commands.py")}
		args = append(args, os.Args[1:]...)
		cmd := exec.Command("python3", args...)
		cmd.Dir = root
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
		cmd.Stdin = os.Stdin
		_ = validateTimeout
		_ = validateJobs
		if err := cmd.Run(); err != nil {
			var exit *exec.ExitError
			if errors.As(err, &exit) {
				os.Exit(exit.ExitCode())
			}
			fatal(err)
		}
		return
	}

	inputs := flags.Args()
	databases, err := databaseInputs(root, inputs)
	if err != nil {
		fatal(err)
	}
	commands, err := mergeDatabases(databases)
	if err != nil {
		fatal(err)
	}
	headerCount := 0
	if !noArcHeaders {
		headers, err := arcHeaderCommands(root, commands)
		if err != nil {
			fatal(err)
		}
		headerCount = len(headers)
		commands = append(commands, headers...)
	}

	out := output
	if !filepath.IsAbs(out) {
		out = filepath.Join(root, out)
	}
	if err := os.MkdirAll(filepath.Dir(out), 0o755); err != nil {
		fatal(err)
	}
	data, err := json.MarshalIndent(commands, "", "  ")
	if err != nil {
		fatal(err)
	}
	data = append(data, '\n')
	if err := os.WriteFile(out, data, 0o644); err != nil {
		fatal(err)
	}
	rel := display(root, out)
	fmt.Printf("merged %d commands from %d databases into %s (%d inferred Arc header commands)\n",
		len(commands), len(databases), rel, headerCount)
}

func repoRoot() (string, error) {
	wd, err := os.Getwd()
	if err != nil {
		return "", err
	}
	for {
		if _, err := os.Stat(filepath.Join(wd, ".git")); err == nil {
			return wd, nil
		}
		next := filepath.Dir(wd)
		if next == wd {
			return "", fmt.Errorf("not inside a git repository")
		}
		wd = next
	}
}

func databaseInputs(root string, inputs []string) ([]string, error) {
	if len(inputs) > 0 {
		var out []string
		for _, input := range inputs {
			path := input
			if !filepath.IsAbs(path) {
				path = filepath.Join(root, path)
			}
			info, err := os.Stat(path)
			if err == nil && info.IsDir() {
				path = filepath.Join(path, "build", "compile_commands.json")
			}
			if _, err := os.Stat(path); err != nil {
				return nil, fmt.Errorf("missing compile database: %s", display(root, path))
			}
			out = append(out, path)
		}
		return out, nil
	}

	var dbs []string
	projects := []string{root}
	examples := filepath.Join(root, "examples")
	if entries, err := os.ReadDir(examples); err == nil {
		for _, entry := range entries {
			if entry.IsDir() {
				project := filepath.Join(examples, entry.Name())
				if _, err := os.Stat(filepath.Join(project, "CMakeLists.txt")); err == nil {
					projects = append(projects, project)
				}
			}
		}
	}
	for _, project := range projects {
		if db := newestDatabase(project); db != "" {
			dbs = append(dbs, db)
		}
	}
	if len(dbs) == 0 {
		return nil, fmt.Errorf("no ESP-IDF compile databases found; run idf.py reconfigure or idf.py build")
	}
	return dbs, nil
}

func newestDatabase(project string) string {
	var best string
	var bestMod int64
	candidates, _ := filepath.Glob(filepath.Join(project, "build*", "compile_commands.json"))
	for _, candidate := range candidates {
		info, err := os.Stat(candidate)
		if err == nil && info.ModTime().UnixNano() > bestMod {
			best = candidate
			bestMod = info.ModTime().UnixNano()
		}
	}
	return best
}

func mergeDatabases(databases []string) ([]commandEntry, error) {
	seen := map[string]bool{}
	var merged []commandEntry
	for _, db := range databases {
		data, err := os.ReadFile(db)
		if err != nil {
			return nil, err
		}
		var entries []commandEntry
		if err := json.Unmarshal(data, &entries); err != nil {
			return nil, fmt.Errorf("%s is not valid JSON: %w", db, err)
		}
		for _, entry := range entries {
			key := entrySource(entry, filepath.Dir(db))
			if key == "" {
				key = entry.File
			}
			if seen[key] {
				continue
			}
			seen[key] = true
			merged = append(merged, entry)
		}
	}
	return merged, nil
}

func arcHeaderCommands(root string, commands []commandEntry) ([]commandEntry, error) {
	base := firstCxxCommand(root, commands)
	if base == nil {
		return nil, nil
	}
	include := filepath.Join(root, "components", "arc", "include")
	var headers []string
	if err := filepath.WalkDir(include, func(path string, entry os.DirEntry, err error) error {
		if err != nil {
			return err
		}
		if entry.IsDir() {
			return nil
		}
		switch strings.ToLower(filepath.Ext(path)) {
		case ".h", ".hh", ".hpp", ".hxx":
			headers = append(headers, path)
		}
		return nil
	}); err != nil {
		return nil, err
	}
	sort.Strings(headers)
	out := make([]commandEntry, 0, len(headers))
	for _, header := range headers {
		clone := cloneCommand(*base)
		replaceFile(&clone, header)
		out = append(out, clone)
	}
	return out, nil
}

func firstCxxCommand(root string, commands []commandEntry) *commandEntry {
	var best *commandEntry
	bestRank := 1 << 30
	for i := range commands {
		source := entrySource(commands[i], root)
		ext := strings.ToLower(filepath.Ext(source))
		if ext != ".cc" && ext != ".cpp" && ext != ".cxx" && ext != ".c++" {
			continue
		}
		rank := 100
		if filepath.Base(source) == "app_main.cpp" {
			rank -= 20
		}
		if strings.HasPrefix(source, filepath.Join(root, "main")) {
			rank -= 20
		}
		if strings.Contains(source, string(filepath.Separator)+"examples"+string(filepath.Separator)) {
			rank += 10
		}
		if rank < bestRank {
			bestRank = rank
			best = &commands[i]
		}
	}
	return best
}

func entrySource(entry commandEntry, fallback string) string {
	if entry.File == "" {
		return ""
	}
	if filepath.IsAbs(entry.File) {
		return filepath.Clean(entry.File)
	}
	base := entry.Directory
	if base == "" {
		base = fallback
	}
	return filepath.Clean(filepath.Join(base, entry.File))
}

func cloneCommand(entry commandEntry) commandEntry {
	clone := entry
	if entry.Arguments != nil {
		clone.Arguments = append([]string(nil), entry.Arguments...)
	}
	if entry.extra != nil {
		clone.extra = map[string]any{}
		for k, v := range entry.extra {
			clone.extra[k] = v
		}
	}
	return clone
}

func replaceFile(entry *commandEntry, file string) {
	old := entry.File
	entry.File = file
	if len(entry.Arguments) > 0 {
		for i, arg := range entry.Arguments {
			if arg == old || filepath.Clean(arg) == filepath.Clean(old) {
				entry.Arguments[i] = file
			}
		}
	}
	if entry.Command != "" {
		entry.Command = strings.ReplaceAll(entry.Command, old, file)
	}
}

func display(root, path string) string {
	if rel, err := filepath.Rel(root, path); err == nil && !strings.HasPrefix(rel, "..") {
		return filepath.ToSlash(rel)
	}
	return path
}

func fatal(err error) {
	fmt.Fprintln(os.Stderr, err)
	os.Exit(1)
}
