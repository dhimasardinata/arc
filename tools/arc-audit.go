//go:build ignore

package main

import (
	"flag"
	"fmt"
	"os"
	"path/filepath"
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

const defaultForbidden = "malloc,calloc,realloc,free,heap_caps_malloc,heap_caps_calloc,heap_caps_realloc,heap_caps_free,new,delete,virtual,vTaskDelay,vTaskDelayUntil,vTaskSuspend,xSemaphoreTake,xSemaphoreTakeRecursive,xQueueReceive,xQueuePeek,xQueueSemaphoreTake,xQueueSend,xQueueSendToBack,xQueueSendToFront,xEventGroupWaitBits,xEventGroupSync,ulTaskNotifyTake,ulTaskNotifyTakeIndexed,xTaskNotifyWait,xTaskNotifyWaitIndexed,xTimerPendFunctionCall,ESP_LOGE,ESP_LOGW,ESP_LOGI,ESP_LOGD,ESP_LOGV,printf,puts"

func main() {
	root := flag.String("root", ".", "repository root")
	all := flag.Bool("all", false, "scan every source file instead of annotated realtime entries")
	forbid := flag.String("forbid", defaultForbidden, "comma-separated forbidden realtime tokens")
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
		byName := map[string][]function{}
		for _, fn := range funcs {
			byName[fn.Name] = append(byName[fn.Name], fn)
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
			if skipSourcePart(info.Name()) {
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

func skipSourcePart(part string) bool {
	switch part {
	case ".git", ".cache", ".codex", ".espressif", ".ruff_cache", "build", "dump", "managed_components", "esp-idf", "arduino-esp32":
		return true
	default:
		return strings.HasPrefix(part, "build-")
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
	inString := byte(0)
	escape := false
	for i := 0; i < len(text); i++ {
		ch := text[i]
		next := byte(0)
		if i+1 < len(text) {
			next = text[i+1]
		}
		switch {
		case inLine:
			if ch == '\n' {
				inLine = false
				b.WriteByte('\n')
			} else {
				b.WriteByte(' ')
			}
		case inBlock:
			if ch == '*' && next == '/' {
				inBlock = false
				b.WriteString("  ")
				i++
			} else if ch == '\n' {
				b.WriteByte('\n')
			} else {
				b.WriteByte(' ')
			}
		case inString != 0:
			if ch == '\n' {
				b.WriteByte('\n')
				escape = false
				continue
			}
			if escape {
				escape = false
			} else if ch == '\\' {
				escape = true
			} else if ch == inString {
				inString = 0
			}
			b.WriteByte(' ')
		case ch == '/' && next == '/':
			inLine = true
			b.WriteString("  ")
			i++
		case ch == '/' && next == '*':
			inBlock = true
			b.WriteString("  ")
			i++
		case ch == '"' || ch == '\'':
			inString = ch
			b.WriteByte(' ')
		default:
			b.WriteByte(ch)
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
	if strings.Contains(sig, "->*") {
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

func walkCallGraph(path string, entry function, funcs map[string][]function, tokens []string) []violation {
	seen := map[string]bool{}
	var out []violation
	var visit func(function)
	visit = func(fn function) {
		key := fmt.Sprintf("%s:%d", fn.Name, fn.Line)
		if seen[key] {
			return
		}
		seen[key] = true
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
		for _, name := range callNames(fn.Body) {
			if isKeyword(name) {
				continue
			}
			for _, next := range funcs[name] {
				visit(next)
			}
		}
	}
	visit(entry)
	return out
}

func callNames(body string) []string {
	var out []string
	for pos := 0; pos < len(body); pos++ {
		if !isIdentStart(body[pos]) || (pos > 0 && isIdent(body[pos-1])) {
			continue
		}
		end := pos + 1
		for end < len(body) && isIdent(body[end]) {
			end++
		}
		next := nextNonSpace(body, end)
		if next < len(body) && body[next] == '<' {
			if templateEnd := skipTemplateArgs(body, next); templateEnd >= 0 {
				next = nextNonSpace(body, templateEnd+1)
			}
		}
		if next < len(body) && body[next] == '(' {
			out = append(out, body[pos:end])
		}
		pos = end
	}
	return out
}

func nextNonSpace(text string, pos int) int {
	for pos < len(text) {
		switch text[pos] {
		case ' ', '\t', '\r', '\n':
			pos++
		default:
			return pos
		}
	}
	return pos
}

func skipTemplateArgs(text string, open int) int {
	depth := 0
	for pos := open; pos < len(text); pos++ {
		switch text[pos] {
		case '<':
			depth++
		case '>':
			depth--
			if depth == 0 {
				return pos
			}
		}
	}
	return -1
}

func containsToken(body string, token string) bool {
	if token == "new" || token == "delete" || token == "virtual" {
		return containsWord(body, token)
	}
	return containsCall(body, token)
}

func containsCall(body string, name string) bool {
	for offset := 0; ; {
		idx := strings.Index(body[offset:], name)
		if idx < 0 {
			return false
		}
		idx += offset
		before := idx == 0 || !isIdent(body[idx-1])
		after := idx+len(name) >= len(body) || !isIdent(body[idx+len(name)])
		if before && after {
			next := nextNonSpace(body, idx+len(name))
			if next < len(body) && body[next] == '<' {
				if templateEnd := skipTemplateArgs(body, next); templateEnd >= 0 {
					next = nextNonSpace(body, templateEnd+1)
				}
			}
			if next < len(body) && body[next] == '(' {
				return true
			}
		}
		offset = idx + len(name)
	}
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

func isIdentStart(c byte) bool {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'
}
