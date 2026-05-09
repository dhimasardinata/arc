//go:build ignore

package main

import (
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"regexp"
	"strings"
)

type function struct {
	Name string
	Body string
	Line int
}

type violation struct {
	File  string
	Line  int
	Entry string
	Token string
}

var callRE = regexp.MustCompile(`\b([A-Za-z_][A-Za-z0-9_]*)\s*\(`)

func main() {
	root := flag.String("root", ".", "repository root")
	all := flag.Bool("all", false, "scan every source file instead of annotated realtime entries")
	forbid := flag.String("forbid", "malloc,heap_caps_malloc,new,vTaskDelay,xSemaphoreTake,virtual", "comma-separated forbidden realtime tokens")
	flag.Parse()

	tokens := splitCSV(*forbid)
	var violations []violation
	entries := 0
	for _, path := range walkSources(*root) {
		data, err := os.ReadFile(path)
		if err != nil {
			fmt.Fprintf(os.Stderr, "arc-audit: %v\n", err)
			os.Exit(1)
		}
		text := string(data)
		if !*all && !isRealtimeCandidate(text) {
			continue
		}
		clean := strip(text)
		funcs := parseFunctions(clean)
		byName := map[string]function{}
		for _, fn := range funcs {
			byName[fn.Name] = fn
		}
		for _, fn := range funcs {
			if fn.Name != "loop" && fn.Name != "step" {
				continue
			}
			entries++
			violations = append(violations, walkCallGraph(path, fn, byName, tokens)...)
		}
	}

	if len(violations) != 0 {
		for _, item := range violations {
			fmt.Fprintf(os.Stderr, "%s:%d: realtime entry %s reaches forbidden token %q\n",
				item.File, item.Line, item.Entry, item.Token)
		}
		os.Exit(1)
	}
	fmt.Printf("arc-audit: checked %d realtime entries, 0 violations\n", entries)
}

func walkSources(root string) []string {
	var files []string
	_ = filepath.Walk(root, func(path string, info os.FileInfo, err error) error {
		if err != nil || info == nil {
			return nil
		}
		if info.IsDir() {
			switch info.Name() {
			case ".git", ".cache", ".codex", ".espressif", ".ruff_cache", "build", "dump", "managed_components", "esp-idf", "arduino-esp32":
				return filepath.SkipDir
			}
			return nil
		}
		if isSource(path) {
			files = append(files, path)
		}
		return nil
	})
	return files
}

func isSource(path string) bool {
	switch filepath.Ext(path) {
	case ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx":
		return true
	default:
		return false
	}
}

func splitCSV(text string) []string {
	var out []string
	for _, item := range strings.Split(text, ",") {
		item = strings.TrimSpace(item)
		if item != "" {
			out = append(out, item)
		}
	}
	return out
}

func isRealtimeCandidate(text string) bool {
	return strings.Contains(text, "ARC_REALTIME_PROOF") ||
		strings.Contains(text, "arc::Tight<") ||
		strings.Contains(text, "arc::Hard<") ||
		(strings.Contains(text, "arc::App<") && strings.Contains(text, "arc::Core::core1"))
}

func strip(text string) string {
	var b strings.Builder
	b.Grow(len(text))
	inLine := false
	inBlock := false
	inString := rune(0)
	escape := false
	for i, r := range text {
		next := byte(0)
		if i+1 < len(text) {
			next = text[i+1]
		}
		switch {
		case inLine:
			if r == '\n' {
				inLine = false
				b.WriteRune('\n')
			} else {
				b.WriteByte(' ')
			}
		case inBlock:
			if r == '*' && next == '/' {
				inBlock = false
				b.WriteString("  ")
			} else if r == '\n' {
				b.WriteRune('\n')
			} else {
				b.WriteByte(' ')
			}
		case inString != 0:
			if r == '\n' {
				b.WriteRune('\n')
				escape = false
				continue
			}
			if escape {
				escape = false
			} else if r == '\\' {
				escape = true
			} else if r == inString {
				inString = 0
			}
			b.WriteByte(' ')
		case r == '/' && next == '/':
			inLine = true
			b.WriteString("  ")
		case r == '/' && next == '*':
			inBlock = true
			b.WriteString("  ")
		case r == '"' || r == '\'':
			inString = r
			b.WriteByte(' ')
		default:
			b.WriteRune(r)
		}
	}
	return b.String()
}

func parseFunctions(text string) []function {
	var out []function
	for pos := 0; pos < len(text); pos++ {
		if text[pos] != '{' {
			continue
		}
		sigStart := signatureStart(text, pos)
		sig := strings.TrimSpace(text[sigStart:pos])
		name := functionName(sig)
		if name == "" {
			continue
		}
		end := matchBrace(text, pos)
		if end < 0 {
			continue
		}
		out = append(out, function{
			Name: name,
			Body: text[pos+1 : end],
			Line: strings.Count(text[:pos], "\n") + 1,
		})
		pos = end
	}
	return out
}

func signatureStart(text string, pos int) int {
	start := pos
	for start > 0 {
		prev := text[start-1]
		if prev == ';' || prev == '}' || prev == '{' {
			break
		}
		start--
	}
	return start
}

func functionName(sig string) string {
	if strings.Contains(sig, "=") || strings.Contains(sig, "->*") {
		return ""
	}
	open := strings.LastIndex(sig, "(")
	if open < 0 {
		return ""
	}
	head := strings.TrimSpace(sig[:open])
	if head == "" || strings.HasSuffix(head, "if") || strings.HasSuffix(head, "for") ||
		strings.HasSuffix(head, "while") || strings.HasSuffix(head, "switch") {
		return ""
	}
	end := len(head)
	for end > 0 && isIdent(head[end-1]) {
		end--
	}
	name := head[end:]
	if name == "" {
		return ""
	}
	return name
}

func matchBrace(text string, open int) int {
	depth := 0
	for i := open; i < len(text); i++ {
		switch text[i] {
		case '{':
			depth++
		case '}':
			depth--
			if depth == 0 {
				return i
			}
		}
	}
	return -1
}

func walkCallGraph(path string, entry function, funcs map[string]function, tokens []string) []violation {
	seen := map[string]bool{}
	var out []violation
	var visit func(function)
	visit = func(fn function) {
		if seen[fn.Name] {
			return
		}
		seen[fn.Name] = true
		for _, token := range tokens {
			if containsToken(fn.Body, token) {
				out = append(out, violation{
					File:  path,
					Line:  fn.Line,
					Entry: entry.Name,
					Token: token,
				})
			}
		}
		for _, match := range callRE.FindAllStringSubmatch(fn.Body, -1) {
			name := match[1]
			if isKeyword(name) {
				continue
			}
			if next, ok := funcs[name]; ok {
				visit(next)
			}
		}
	}
	visit(entry)
	return out
}

func containsToken(body string, token string) bool {
	if token == "new" || token == "virtual" {
		return containsWord(body, token)
	}
	return strings.Contains(body, token+"(") || strings.Contains(body, token+" <")
}

func containsWord(body string, word string) bool {
	for offset := 0; ; {
		idx := strings.Index(body[offset:], word)
		if idx < 0 {
			return false
		}
		idx += offset
		before := idx == 0 || !isIdent(body[idx-1])
		after := idx+len(word) >= len(body) || !isIdent(body[idx+len(word)])
		if before && after {
			return true
		}
		offset = idx + len(word)
	}
}

func isKeyword(name string) bool {
	switch name {
	case "if", "for", "while", "switch", "return", "sizeof", "static_cast", "reinterpret_cast", "const_cast", "std":
		return true
	default:
		return false
	}
}

func isIdent(c byte) bool {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'
}
