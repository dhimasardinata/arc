//go:build ignore

package main

import (
	"errors"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
)

func main() {
	root, err := repoRoot()
	if err != nil {
		fatal(err)
	}

	args := append([]string{filepath.Join(root, "tools", "clangd-compile-commands.py")}, os.Args[1:]...)
	cmd := exec.Command("python3", args...)
	cmd.Dir = root
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Stdin = os.Stdin
	if err := cmd.Run(); err != nil {
		var exit *exec.ExitError
		if errors.As(err, &exit) {
			os.Exit(exit.ExitCode())
		}
		fatal(err)
	}
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

func fatal(err error) {
	fmt.Fprintln(os.Stderr, err)
	os.Exit(1)
}
