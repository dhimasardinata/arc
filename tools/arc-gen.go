//go:build ignore

package main

import (
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

type field struct {
	Name string
	Cpp  string
	TS   string
	Wire string
}

type reflected struct {
	Type   string
	Source string
	Fields []field
}

func main() {
	root := flag.String("root", ".", "repository root")
	compileDB := flag.String("compile-db", "compile_commands.json", "optional compile database to seed source discovery")
	apiTS := flag.String("api-ts", "", "optional generated TypeScript API path")
	schemaGo := flag.String("schema-go", "", "optional generated Go bridge schema path")
	flag.Parse()

	files := unique(append(filesFromCompileDB(filepath.Join(*root, *compileDB)), walkSources(*root)...))

	var reflectedTypes []reflected
	for _, file := range files {
		decls, err := scanFile(file)
		if err != nil {
			fmt.Fprintf(os.Stderr, "arc-gen: %v\n", err)
			os.Exit(1)
		}
		reflectedTypes = append(reflectedTypes, decls...)
	}

	if *apiTS == "" && *schemaGo == "" {
		for _, item := range reflectedTypes {
			fmt.Printf("%s %s", item.Type, schemaString(item))
			if item.Source != "" {
				fmt.Printf(" %s", item.Source)
			}
			fmt.Println()
		}
		return
	}
	if *apiTS != "" {
		if err := writeFile(*apiTS, tsOutput(reflectedTypes)); err != nil {
			fmt.Fprintf(os.Stderr, "arc-gen: %v\n", err)
			os.Exit(1)
		}
	}
	if *schemaGo != "" {
		if err := writeFile(*schemaGo, goOutput(reflectedTypes)); err != nil {
			fmt.Fprintf(os.Stderr, "arc-gen: %v\n", err)
			os.Exit(1)
		}
	}
}

func filesFromCompileDB(path string) []string {
	data, err := os.ReadFile(path)
	if err != nil {
		return nil
	}
	text := string(data)
	var files []string
	for offset := 0; ; {
		index := strings.Index(text[offset:], `"file"`)
		if index < 0 {
			break
		}
		offset += index + len(`"file"`)
		colon := strings.IndexByte(text[offset:], ':')
		if colon < 0 {
			break
		}
		offset += colon + 1
		value, next, ok := readJSONString(text[offset:])
		if !ok {
			continue
		}
		offset += next
		if isArcSource(value) {
			files = append(files, value)
		}
	}
	return files
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
		if isArcSource(path) {
			files = append(files, path)
		}
		return nil
	})
	return files
}

func scanFile(path string) ([]reflected, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		if os.IsNotExist(err) {
			return nil, nil
		}
		return nil, err
	}
	text := string(data)
	calls := reflectCalls(text)
	if len(calls) == 0 {
		return nil, nil
	}

	var out []reflected
	for _, call := range calls {
		parts := splitArgs(call)
		if len(parts) < 2 {
			continue
		}
		item := reflected{Type: strings.TrimSpace(parts[0]), Source: path}
		members := structFields(text, item.Type)
		for _, part := range parts[1:] {
			name := fieldName(part)
			cpp := members[name]
			item.Fields = append(item.Fields, field{
				Name: name,
				Cpp:  cpp,
				TS:   tsType(cpp),
				Wire: wireType(cpp),
			})
		}
		out = append(out, item)
	}
	return out, nil
}

func reflectCalls(text string) []string {
	const marker = "ARC_PACK_REFLECT("
	var calls []string
	for offset := 0; ; {
		index := strings.Index(text[offset:], marker)
		if index < 0 {
			return calls
		}
		start := offset + index + len(marker)
		lineStart := strings.LastIndexByte(text[:start], '\n') + 1
		if strings.Contains(text[lineStart:start], "#define") {
			offset = start
			continue
		}
		depth := 1
		for pos := start; pos < len(text); pos++ {
			switch text[pos] {
			case '(':
				depth++
			case ')':
				depth--
				if depth == 0 {
					calls = append(calls, text[start:pos])
					offset = pos + 1
					goto next
				}
			}
		}
		return calls
	next:
	}
}

func splitArgs(text string) []string {
	var args []string
	start := 0
	depth := 0
	for i, ch := range text {
		switch ch {
		case '(', '<':
			depth++
		case ')', '>':
			if depth > 0 {
				depth--
			}
		case ',':
			if depth == 0 {
				args = append(args, strings.TrimSpace(text[start:i]))
				start = i + 1
			}
		}
	}
	args = append(args, strings.TrimSpace(text[start:]))
	return args
}

func structFields(text string, name string) map[string]string {
	fields := map[string]string{}
	index := strings.Index(text, "struct "+name)
	if index < 0 {
		return fields
	}
	open := strings.IndexByte(text[index:], '{')
	if open < 0 {
		return fields
	}
	bodyStart := index + open + 1
	depth := 1
	for pos := bodyStart; pos < len(text); pos++ {
		switch text[pos] {
		case '{':
			depth++
		case '}':
			depth--
			if depth == 0 {
				for _, line := range strings.Split(text[bodyStart:pos], "\n") {
					cpp, field, ok := parseField(line)
					if ok {
						fields[field] = cpp
					}
				}
				return fields
			}
		}
	}
	return fields
}

func parseField(line string) (string, string, bool) {
	line = strings.TrimSpace(strings.Split(line, "//")[0])
	if !strings.Contains(line, ";") || strings.Contains(line, "(") {
		return "", "", false
	}
	before := strings.Split(line, ";")[0]
	before = strings.Split(before, "{")[0]
	before = strings.Split(before, "=")[0]
	parts := strings.Fields(strings.TrimSpace(before))
	if len(parts) < 2 {
		return "", "", false
	}
	name := strings.Trim(parts[len(parts)-1], "*&")
	cpp := strings.Join(parts[:len(parts)-1], " ")
	return cpp, name, true
}

func fieldName(text string) string {
	text = strings.TrimSpace(text)
	if idx := strings.LastIndex(text, "::"); idx >= 0 {
		text = text[idx+2:]
	}
	text = strings.Trim(text, " )\t\r\n")
	return text
}

func tsOutput(items []reflected) string {
	var b strings.Builder
	b.WriteString("// Generated by tools/arc-gen.go. Do not edit.\n\n")
	for _, item := range items {
		fmt.Fprintf(&b, "export interface %s {\n", item.Type)
		for _, field := range item.Fields {
			fmt.Fprintf(&b, "  %s: %s;\n", field.Name, field.TS)
		}
		fmt.Fprintf(&b, "}\n\nexport const %sSchema = %q;\n\n", item.Type, schemaString(item))
	}
	return b.String()
}

func goOutput(items []reflected) string {
	var b strings.Builder
	b.WriteString("// Generated by tools/arc-gen.go. Do not edit.\n\npackage schema\n\n")
	b.WriteString("type Field struct {\n\tName string\n\tKind string\n}\n\n")
	b.WriteString("type Schema struct {\n\tName string\n\tFields []Field\n}\n\n")
	b.WriteString("var Schemas = []Schema{\n")
	for _, item := range items {
		fmt.Fprintf(&b, "\t{Name: %q, Fields: []Field{", item.Type)
		for index, field := range item.Fields {
			if index != 0 {
				b.WriteString(", ")
			}
			fmt.Fprintf(&b, "{Name: %q, Kind: %q}", field.Name, field.Wire)
		}
		b.WriteString("}},\n")
	}
	b.WriteString("}\n")
	return b.String()
}

func schemaString(item reflected) string {
	parts := make([]string, 0, len(item.Fields))
	for _, field := range item.Fields {
		parts = append(parts, field.Wire+":"+field.Name)
	}
	return strings.Join(parts, ",")
}

func wireType(cpp string) string {
	switch strings.TrimSpace(cpp) {
	case "std::uint8_t", "uint8_t":
		return "u8"
	case "std::int8_t", "int8_t":
		return "i8"
	case "std::uint16_t", "uint16_t":
		return "u16"
	case "std::int16_t", "int16_t":
		return "i16"
	case "std::uint32_t", "uint32_t":
		return "u32"
	case "std::int32_t", "int32_t":
		return "i32"
	case "std::uint64_t", "uint64_t":
		return "u64"
	case "std::int64_t", "int64_t":
		return "i64"
	case "float":
		return "f32"
	case "double":
		return "f64"
	default:
		return "raw"
	}
}

func tsType(cpp string) string {
	if strings.TrimSpace(cpp) == "bool" {
		return "boolean"
	}
	return "number"
}

func writeFile(path string, content string) error {
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	return os.WriteFile(path, []byte(content), 0o644)
}

func unique(files []string) []string {
	seen := map[string]struct{}{}
	out := make([]string, 0, len(files))
	for _, file := range files {
		clean := filepath.Clean(file)
		if _, ok := seen[clean]; ok {
			continue
		}
		seen[clean] = struct{}{}
		out = append(out, clean)
	}
	return out
}

func isSource(path string) bool {
	switch filepath.Ext(path) {
	case ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx":
		return true
	default:
		return false
	}
}

func isArcSource(path string) bool {
	if !isSource(path) {
		return false
	}
	clean := filepath.ToSlash(path)
	for _, skip := range []string{"/esp-idf/", "/arduino-esp32/", "/managed_components/", "/build/", "/dump/", "/.espressif/"} {
		if strings.Contains(clean, skip) {
			return false
		}
	}
	return true
}

func readJSONString(text string) (string, int, bool) {
	start := strings.IndexByte(text, '"')
	if start < 0 {
		return "", 0, false
	}
	var b strings.Builder
	for pos := start + 1; pos < len(text); pos++ {
		if text[pos] == '"' {
			return b.String(), pos + 1, true
		}
		if text[pos] == '\\' && pos+1 < len(text) {
			pos++
		}
		b.WriteByte(text[pos])
	}
	return "", 0, false
}
