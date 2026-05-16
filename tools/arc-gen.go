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

type compileCommand struct {
	Directory string `json:"directory"`
	File      string `json:"file"`
}

type memberDecl struct {
	Cpp  string
	Name string
}

type jsonScanner struct {
	text string
	pos  int
}

func main() {
	root := flag.String("root", ".", "repository root")
	compileDB := flag.String("compile-db", "compile_commands.json", "optional compile database to seed source discovery")
	apiTS := flag.String("api-ts", "", "optional generated TypeScript API path")
	schemaGo := flag.String("schema-go", "", "optional generated Go bridge schema path")
	flag.Parse()

	dbFiles, err := filesFromCompileDB(resolvePath(*root, *compileDB))
	if err != nil {
		fmt.Fprintf(os.Stderr, "arc-gen: %v\n", err)
		os.Exit(1)
	}
	files := unique(append(dbFiles, walkSources(*root)...))

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

func filesFromCompileDB(path string) ([]string, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		if os.IsNotExist(err) {
			return nil, nil
		}
		return nil, err
	}
	var entries []compileCommand
	entries, err = parseCompileCommands(string(data))
	if err != nil {
		return nil, fmt.Errorf("%s: %w", path, err)
	}
	var files []string
	dbDir := filepath.Dir(path)
	for _, entry := range entries {
		if entry.File == "" {
			continue
		}
		file := entry.File
		directory := entry.Directory
		if directory != "" && !filepath.IsAbs(directory) {
			directory = filepath.Join(dbDir, directory)
		}
		if !filepath.IsAbs(file) && directory != "" {
			file = filepath.Join(directory, file)
		} else if !filepath.IsAbs(file) {
			file = filepath.Join(dbDir, file)
		}
		if isArcSource(file) {
			files = append(files, file)
		}
	}
	return files, nil
}

func resolvePath(root string, path string) string {
	if filepath.IsAbs(path) {
		return path
	}
	return filepath.Join(root, path)
}

func parseCompileCommands(text string) ([]compileCommand, error) {
	scan := jsonScanner{text: text}
	scan.skipSpace()
	if !scan.consume('[') {
		return nil, scan.err("expected compile_commands array")
	}
	scan.skipSpace()
	if scan.consume(']') {
		scan.skipSpace()
		if scan.pos != len(scan.text) {
			return nil, scan.err("unexpected trailing data")
		}
		return nil, nil
	}

	var out []compileCommand
	for {
		entry, err := scan.readCompileCommand()
		if err != nil {
			return nil, err
		}
		out = append(out, entry)
		scan.skipSpace()
		if scan.consume(']') {
			scan.skipSpace()
			if scan.pos != len(scan.text) {
				return nil, scan.err("unexpected trailing data")
			}
			return out, nil
		}
		if !scan.consume(',') {
			return nil, scan.err("expected ',' or ']'")
		}
	}
}

func (scan *jsonScanner) readCompileCommand() (compileCommand, error) {
	scan.skipSpace()
	if !scan.consume('{') {
		return compileCommand{}, scan.err("expected compile command object")
	}
	scan.skipSpace()
	if scan.consume('}') {
		return compileCommand{}, nil
	}

	var out compileCommand
	for {
		key, err := scan.readString()
		if err != nil {
			return compileCommand{}, err
		}
		scan.skipSpace()
		if !scan.consume(':') {
			return compileCommand{}, scan.err("expected ':'")
		}
		switch key {
		case "directory":
			value, err := scan.readString()
			if err != nil {
				return compileCommand{}, err
			}
			out.Directory = value
		case "file":
			value, err := scan.readString()
			if err != nil {
				return compileCommand{}, err
			}
			out.File = value
		default:
			if err := scan.skipValue(); err != nil {
				return compileCommand{}, err
			}
		}
		scan.skipSpace()
		if scan.consume('}') {
			return out, nil
		}
		if !scan.consume(',') {
			return compileCommand{}, scan.err("expected ',' or '}'")
		}
	}
}

func (scan *jsonScanner) skipValue() error {
	scan.skipSpace()
	if scan.pos >= len(scan.text) {
		return scan.err("expected value")
	}
	switch scan.text[scan.pos] {
	case '"':
		_, err := scan.readString()
		return err
	case '{':
		scan.pos++
		scan.skipSpace()
		if scan.consume('}') {
			return nil
		}
		for {
			if _, err := scan.readString(); err != nil {
				return err
			}
			scan.skipSpace()
			if !scan.consume(':') {
				return scan.err("expected ':'")
			}
			if err := scan.skipValue(); err != nil {
				return err
			}
			scan.skipSpace()
			if scan.consume('}') {
				return nil
			}
			if !scan.consume(',') {
				return scan.err("expected ',' or '}'")
			}
		}
	case '[':
		scan.pos++
		scan.skipSpace()
		if scan.consume(']') {
			return nil
		}
		for {
			if err := scan.skipValue(); err != nil {
				return err
			}
			scan.skipSpace()
			if scan.consume(']') {
				return nil
			}
			if !scan.consume(',') {
				return scan.err("expected ',' or ']'")
			}
		}
	default:
		return scan.skipAtom()
	}
}

func (scan *jsonScanner) skipAtom() error {
	start := scan.pos
	for scan.pos < len(scan.text) && !strings.ContainsRune(",}] \t\r\n", rune(scan.text[scan.pos])) {
		scan.pos++
	}
	if scan.pos == start {
		return scan.err("expected value")
	}
	token := scan.text[start:scan.pos]
	switch token {
	case "true", "false", "null":
		return nil
	}
	if validJSONNumber(token) {
		return nil
	}
	return scan.err("invalid JSON literal")
}

func validJSONNumber(text string) bool {
	if text == "" {
		return false
	}
	pos := 0
	if text[pos] == '-' {
		pos++
		if pos == len(text) {
			return false
		}
	}
	if text[pos] == '0' {
		pos++
	} else if text[pos] >= '1' && text[pos] <= '9' {
		for pos < len(text) && text[pos] >= '0' && text[pos] <= '9' {
			pos++
		}
	} else {
		return false
	}
	if pos < len(text) && text[pos] == '.' {
		pos++
		start := pos
		for pos < len(text) && text[pos] >= '0' && text[pos] <= '9' {
			pos++
		}
		if pos == start {
			return false
		}
	}
	if pos < len(text) && (text[pos] == 'e' || text[pos] == 'E') {
		pos++
		if pos < len(text) && (text[pos] == '+' || text[pos] == '-') {
			pos++
		}
		start := pos
		for pos < len(text) && text[pos] >= '0' && text[pos] <= '9' {
			pos++
		}
		if pos == start {
			return false
		}
	}
	return pos == len(text)
}

func (scan *jsonScanner) readString() (string, error) {
	scan.skipSpace()
	if !scan.consume('"') {
		return "", scan.err("expected string")
	}
	var out strings.Builder
	for scan.pos < len(scan.text) {
		ch := scan.text[scan.pos]
		scan.pos++
		if ch == '\\' {
			if err := scan.writeEscape(&out); err != nil {
				return "", err
			}
			continue
		}
		if ch == '"' {
			return out.String(), nil
		}
		if ch < 0x20 {
			return "", scan.err("control character in string")
		}
		out.WriteByte(ch)
	}
	return "", scan.err("unterminated string")
}

func (scan *jsonScanner) writeEscape(out *strings.Builder) error {
	if scan.pos >= len(scan.text) {
		return scan.err("unterminated escape")
	}
	ch := scan.text[scan.pos]
	scan.pos++
	switch ch {
	case '"', '\\', '/':
		out.WriteByte(ch)
		return nil
	case 'b':
		out.WriteByte('\b')
		return nil
	case 'f':
		out.WriteByte('\f')
		return nil
	case 'n':
		out.WriteByte('\n')
		return nil
	case 'r':
		out.WriteByte('\r')
		return nil
	case 't':
		out.WriteByte('\t')
		return nil
	case 'u':
		value, err := scan.readUnicodeEscape()
		if err != nil {
			return err
		}
		if value >= 0xD800 && value <= 0xDBFF {
			if scan.pos+6 > len(scan.text) || scan.text[scan.pos] != '\\' || scan.text[scan.pos+1] != 'u' {
				return scan.err("missing low unicode surrogate")
			}
			scan.pos += 2
			low, err := scan.readUnicodeEscape()
			if err != nil {
				return err
			}
			if low < 0xDC00 || low > 0xDFFF {
				return scan.err("invalid low unicode surrogate")
			}
			value = 0x10000 + ((value - 0xD800) << 10) + (low - 0xDC00)
		} else if value >= 0xDC00 && value <= 0xDFFF {
			return scan.err("lone low unicode surrogate")
		}
		out.WriteRune(value)
		return nil
	default:
		return scan.err("invalid string escape")
	}
}

func (scan *jsonScanner) readUnicodeEscape() (rune, error) {
	if scan.pos+4 > len(scan.text) {
		return 0, scan.err("short unicode escape")
	}
	value := rune(0)
	for i := 0; i < 4; i++ {
		digit, ok := hexValue(scan.text[scan.pos+i])
		if !ok {
			return 0, scan.err("invalid unicode escape")
		}
		value = (value << 4) | rune(digit)
	}
	scan.pos += 4
	return value, nil
}

func (scan *jsonScanner) skipSpace() {
	for scan.pos < len(scan.text) {
		switch scan.text[scan.pos] {
		case ' ', '\t', '\r', '\n':
			scan.pos++
		default:
			return
		}
	}
}

func (scan *jsonScanner) consume(ch byte) bool {
	if scan.pos < len(scan.text) && scan.text[scan.pos] == ch {
		scan.pos++
		return true
	}
	return false
}

func (scan *jsonScanner) err(message string) error {
	return fmt.Errorf("%s at byte %d", message, scan.pos)
}

func hexValue(c byte) (byte, bool) {
	switch {
	case c >= '0' && c <= '9':
		return c - '0', true
	case c >= 'A' && c <= 'F':
		return c - 'A' + 10, true
	case c >= 'a' && c <= 'f':
		return c - 'a' + 10, true
	default:
		return 0, false
	}
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
	clean := stripCode(string(data))
	calls := reflectCalls(clean)
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
		members := structFields(clean, item.Type)
		for _, part := range parts[1:] {
			name := fieldName(part)
			cpp, ok := members[name]
			if !ok {
				return nil, fmt.Errorf("%s: reflected field %s::%s not found", path, item.Type, name)
			}
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

func stripCode(text string) string {
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
				for _, decl := range strings.Split(text[bodyStart:pos], ";") {
					for _, member := range parseFields(decl + ";") {
						fields[member.Name] = member.Cpp
					}
				}
				return fields
			}
		}
	}
	return fields
}

func parseFields(line string) []memberDecl {
	line = strings.TrimSpace(strings.Split(line, "//")[0])
	if !strings.Contains(line, ";") || strings.Contains(line, "(") {
		return nil
	}
	before := strings.Split(line, ";")[0]
	decls := splitDecls(before)
	if len(decls) == 0 {
		return nil
	}

	cpp, name, ok := parseFirstField(decls[0])
	if !ok {
		return nil
	}

	out := []memberDecl{{Cpp: cpp, Name: name}}
	for _, decl := range decls[1:] {
		name, ok := parseDeclaratorName(decl)
		if ok {
			out = append(out, memberDecl{Cpp: cpp, Name: name})
		}
	}
	return out
}

func parseFirstField(text string) (string, string, bool) {
	clean := declaratorPrefix(text)
	parts := strings.Fields(clean)
	if len(parts) < 2 {
		return "", "", false
	}
	name := strings.Trim(parts[len(parts)-1], "*&")
	cpp := strings.Join(parts[:len(parts)-1], " ")
	return cpp, name, true
}

func parseDeclaratorName(text string) (string, bool) {
	clean := declaratorPrefix(text)
	parts := strings.Fields(clean)
	if len(parts) == 0 {
		return "", false
	}
	name := strings.Trim(parts[len(parts)-1], "*&")
	return name, name != ""
}

func declaratorPrefix(text string) string {
	text = strings.TrimSpace(text)
	for _, delim := range []string{"{", "=", "["} {
		if idx := strings.Index(text, delim); idx >= 0 {
			text = text[:idx]
		}
	}
	return strings.TrimSpace(text)
}

func splitDecls(text string) []string {
	var out []string
	start := 0
	depth := 0
	for i, ch := range text {
		switch ch {
		case '(', '<', '{', '[':
			depth++
		case ')', '>', '}', ']':
			if depth > 0 {
				depth--
			}
		case ',':
			if depth == 0 {
				out = append(out, strings.TrimSpace(text[start:i]))
				start = i + 1
			}
		}
	}
	out = append(out, strings.TrimSpace(text[start:]))
	return out
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
	parts := strings.Split(clean, "/")
	for _, part := range parts[:len(parts)-1] {
		if skipSourcePart(part) {
			return false
		}
	}
	return true
}

func skipSourcePart(part string) bool {
	switch part {
	case ".git", ".cache", ".codex", ".espressif", ".ruff_cache", "build", "dump", "managed_components", "esp-idf", "arduino-esp32":
		return true
	default:
		return strings.HasPrefix(part, "build-")
	}
}
