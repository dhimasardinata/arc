package main

import (
	"bufio"
	"crypto/sha1"
	"encoding/binary"
	"flag"
	"fmt"
	"log"
	"math"
	"net"
	"strconv"
	"strings"
	"sync"
	"time"
)

type field struct {
	kind string
	name string
	size int
}

var typeSize = map[string]int{
	"u8":  1,
	"i8":  1,
	"u16": 2,
	"i16": 2,
	"u32": 4,
	"i32": 4,
	"u64": 8,
	"i64": 8,
	"f32": 4,
	"f64": 8,
}

type hub struct {
	mu      sync.Mutex
	clients map[chan []byte]struct{}
}

func newHub() *hub {
	return &hub{clients: map[chan []byte]struct{}{}}
}

func (h *hub) add() chan []byte {
	ch := make(chan []byte, 64)
	h.mu.Lock()
	h.clients[ch] = struct{}{}
	h.mu.Unlock()
	return ch
}

func (h *hub) remove(ch chan []byte) {
	h.mu.Lock()
	delete(h.clients, ch)
	close(ch)
	h.mu.Unlock()
}

func (h *hub) publish(msg []byte) {
	h.mu.Lock()
	defer h.mu.Unlock()
	for ch := range h.clients {
		select {
		case ch <- msg:
		default:
		}
	}
}

func parseSchema(schema string) ([]field, int, error) {
	var fields []field
	frameSize := 0
	for _, item := range strings.Split(schema, ",") {
		spec := strings.TrimSpace(item)
		if spec == "" {
			continue
		}
		parts := strings.SplitN(spec, ":", 2)
		if len(parts) != 2 {
			return nil, 0, fmt.Errorf("schema field lacks name: %s", spec)
		}
		size, ok := typeSize[parts[0]]
		if !ok {
			return nil, 0, fmt.Errorf("unsupported field type: %s", parts[0])
		}
		fields = append(fields, field{kind: parts[0], name: parts[1], size: size})
		frameSize += size
	}
	if len(fields) == 0 {
		return nil, 0, fmt.Errorf("schema must contain at least one field")
	}
	return fields, frameSize, nil
}

func jsonQuote(text string) string {
	var b strings.Builder
	b.WriteByte('"')
	for _, r := range text {
		switch r {
		case '\\', '"':
			b.WriteByte('\\')
			b.WriteRune(r)
		case '\n':
			b.WriteString(`\n`)
		case '\r':
			b.WriteString(`\r`)
		case '\t':
			b.WriteString(`\t`)
		default:
			if r < 0x20 {
				fmt.Fprintf(&b, `\u%04x`, r)
			} else {
				b.WriteRune(r)
			}
		}
	}
	b.WriteByte('"')
	return b.String()
}

func decodeFrame(fields []field, order binary.ByteOrder, frame []byte) string {
	var b strings.Builder
	b.WriteByte('{')
	pos := 0
	for index, f := range fields {
		if index != 0 {
			b.WriteByte(',')
		}
		b.WriteString(jsonQuote(f.name))
		b.WriteByte(':')
		switch f.kind {
		case "u8":
			b.WriteString(strconv.FormatUint(uint64(frame[pos]), 10))
		case "i8":
			b.WriteString(strconv.FormatInt(int64(int8(frame[pos])), 10))
		case "u16":
			b.WriteString(strconv.FormatUint(uint64(order.Uint16(frame[pos:])), 10))
		case "i16":
			b.WriteString(strconv.FormatInt(int64(int16(order.Uint16(frame[pos:]))), 10))
		case "u32":
			b.WriteString(strconv.FormatUint(uint64(order.Uint32(frame[pos:])), 10))
		case "i32":
			b.WriteString(strconv.FormatInt(int64(int32(order.Uint32(frame[pos:]))), 10))
		case "u64":
			b.WriteString(strconv.FormatUint(order.Uint64(frame[pos:]), 10))
		case "i64":
			b.WriteString(strconv.FormatInt(int64(order.Uint64(frame[pos:])), 10))
		case "f32":
			b.WriteString(strconv.FormatFloat(float64(math.Float32frombits(order.Uint32(frame[pos:]))), 'g', -1, 32))
		case "f64":
			b.WriteString(strconv.FormatFloat(math.Float64frombits(order.Uint64(frame[pos:])), 'g', -1, 64))
		}
		pos += f.size
	}
	b.WriteByte('}')
	return b.String()
}

func schemaJSON(fields []field, frameSize int, endian string) string {
	var b strings.Builder
	b.WriteString(`{"frame_size":`)
	b.WriteString(strconv.Itoa(frameSize))
	b.WriteString(`,"endian":`)
	b.WriteString(jsonQuote(endian))
	b.WriteString(`,"fields":[`)
	for i, f := range fields {
		if i != 0 {
			b.WriteByte(',')
		}
		b.WriteString(`{"name":`)
		b.WriteString(jsonQuote(f.name))
		b.WriteString(`,"type":`)
		b.WriteString(jsonQuote(f.kind))
		b.WriteString(`,"size":`)
		b.WriteString(strconv.Itoa(f.size))
		b.WriteByte('}')
	}
	b.WriteString(`]}`)
	return b.String()
}

func dashboardHTML(fields []field, frameSize int, endian string) string {
	var names strings.Builder
	for i, f := range fields {
		if i != 0 {
			names.WriteByte(',')
		}
		names.WriteString(jsonQuote(f.name))
	}

	var b strings.Builder
	b.WriteString(`<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Arc Bridge</title><style>body{margin:0;font-family:system-ui,sans-serif;background:#101418;color:#e8edf2}main{max-width:960px;margin:0 auto;padding:24px}header{display:flex;justify-content:space-between;gap:16px;align-items:center;margin-bottom:20px}h1{font-size:24px;margin:0}.meta{color:#9ba8b4;font-size:14px}.status{padding:6px 10px;border:1px solid #33414e;border-radius:6px}table{width:100%;border-collapse:collapse;background:#151b21;border:1px solid #2b3641}th,td{text-align:left;padding:10px 12px;border-bottom:1px solid #26323d}th{font-size:12px;color:#aeb9c4;text-transform:uppercase}code{color:#8bd5ca}</style></head><body><main><header><div><h1>Arc Bridge</h1><div class="meta">Frame `)
	b.WriteString(strconv.Itoa(frameSize))
	b.WriteString(` bytes, `)
	b.WriteString(endian)
	b.WriteString(` endian</div></div><div id="status" class="status">connecting</div></header><table><thead><tr><th>Field</th><th>Value</th></tr></thead><tbody id="rows"></tbody></table><p class="meta">Topic <code id="topic">-</code> | Received <code id="seen">0</code></p></main><script>const fields=[`)
	b.WriteString(names.String())
	b.WriteString(`];const rows=document.getElementById("rows");const cells={};for(const field of fields){const tr=document.createElement("tr");const name=document.createElement("td");const value=document.createElement("td");name.textContent=field;value.textContent="-";tr.append(name,value);rows.append(tr);cells[field]=value;}const status=document.getElementById("status");const seen=document.getElementById("seen");const topic=document.getElementById("topic");let count=0;function connect(){const ws=new WebSocket((location.protocol==="https:"?"wss://":"ws://")+location.host+"/ws");ws.onopen=()=>status.textContent="connected";ws.onclose=()=>{status.textContent="reconnecting";setTimeout(connect,1000)};ws.onerror=()=>ws.close();ws.onmessage=(event)=>{const packet=JSON.parse(event.data);count++;seen.textContent=String(count);topic.textContent=packet.topic;for(const field of fields){cells[field].textContent=String(packet.message[field]??"-");}};}connect();</script></body></html>`)
	return b.String()
}

func encodeBase64(src []byte) string {
	const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
	var b strings.Builder
	b.Grow(((len(src) + 2) / 3) * 4)
	for i := 0; i < len(src); i += 3 {
		var block uint32
		remain := len(src) - i
		block |= uint32(src[i]) << 16
		if remain > 1 {
			block |= uint32(src[i+1]) << 8
		}
		if remain > 2 {
			block |= uint32(src[i+2])
		}
		b.WriteByte(alphabet[(block>>18)&0x3f])
		b.WriteByte(alphabet[(block>>12)&0x3f])
		if remain > 1 {
			b.WriteByte(alphabet[(block>>6)&0x3f])
		} else {
			b.WriteByte('=')
		}
		if remain > 2 {
			b.WriteByte(alphabet[block&0x3f])
		} else {
			b.WriteByte('=')
		}
	}
	return b.String()
}

func websocketAccept(key string) string {
	const guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
	sum := sha1.Sum([]byte(key + guid))
	return encodeBase64(sum[:])
}

func writeFrame(conn net.Conn, payload []byte) error {
	header := []byte{0x81}
	switch {
	case len(payload) < 126:
		header = append(header, byte(len(payload)))
	case len(payload) <= 0xffff:
		header = append(header, 126, byte(len(payload)>>8), byte(len(payload)))
	default:
		header = append(header, 127)
		var size [8]byte
		binary.BigEndian.PutUint64(size[:], uint64(len(payload)))
		header = append(header, size[:]...)
	}
	if _, err := conn.Write(header); err != nil {
		return err
	}
	_, err := conn.Write(payload)
	return err
}

func headerValue(line string) (string, string, bool) {
	name, value, ok := strings.Cut(line, ":")
	if !ok {
		return "", "", false
	}
	return strings.ToLower(strings.TrimSpace(name)), strings.TrimSpace(value), true
}

func readHeaders(reader *bufio.Reader) (map[string]string, error) {
	headers := map[string]string{}
	for {
		line, err := reader.ReadString('\n')
		if err != nil {
			return nil, err
		}
		line = strings.TrimRight(line, "\r\n")
		if line == "" {
			return headers, nil
		}
		name, value, ok := headerValue(line)
		if ok {
			headers[name] = value
		}
	}
}

func writeHTTP(conn net.Conn, status string, contentType string, body string) {
	fmt.Fprintf(conn, "HTTP/1.1 %s\r\n", status)
	fmt.Fprintf(conn, "Content-Type: %s\r\n", contentType)
	fmt.Fprintf(conn, "Content-Length: %d\r\n", len(body))
	fmt.Fprintf(conn, "Connection: close\r\n\r\n")
	fmt.Fprint(conn, body)
}

func serveBridgeHTTP(addr string, h *hub, schemaBody string, dashboardBody string) {
	listener, err := net.Listen("tcp", addr)
	if err != nil {
		log.Fatal(err)
	}
	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Print(err)
			continue
		}
		go func(conn net.Conn) {
			defer conn.Close()
			reader := bufio.NewReader(conn)
			request, err := reader.ReadString('\n')
			if err != nil {
				return
			}
			parts := strings.Fields(request)
			if len(parts) < 2 {
				writeHTTP(conn, "400 Bad Request", "text/plain; charset=utf-8", "")
				return
			}
			headers, err := readHeaders(reader)
			if err != nil {
				return
			}
			switch parts[1] {
			case "/":
				writeHTTP(conn, "200 OK", "text/html; charset=utf-8", dashboardBody)
				return
			case "/schema":
				writeHTTP(conn, "200 OK", "application/json", schemaBody)
				return
			case "/ws":
			default:
				writeHTTP(conn, "404 Not Found", "text/plain; charset=utf-8", "not found\n")
				return
			}
			key := headers["sec-websocket-key"]
			if key == "" {
				fmt.Fprintf(conn, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n")
				return
			}
			fmt.Fprintf(conn, "HTTP/1.1 101 Switching Protocols\r\n")
			fmt.Fprintf(conn, "Upgrade: websocket\r\n")
			fmt.Fprintf(conn, "Connection: Upgrade\r\n")
			fmt.Fprintf(conn, "Sec-WebSocket-Accept: %s\r\n\r\n", websocketAccept(key))
			ch := h.add()
			defer h.remove(ch)
			for msg := range ch {
				if err := writeFrame(conn, msg); err != nil {
					return
				}
			}
		}(conn)
	}
}

func main() {
	listen := flag.String("listen", ":7777", "UDP listen address")
	httpAddr := flag.String("http", ":8765", "HTTP/WebSocket listen address")
	schemaText := flag.String("schema", "", "Comma list like u32:seq,i16:temp")
	endian := flag.String("endian", "big", "big or little")
	topic := flag.String("topic", "/arc/telemetry", "Foxglove topic")
	flag.Parse()

	fields, frameSize, err := parseSchema(*schemaText)
	if err != nil {
		log.Fatal(err)
	}
	var order binary.ByteOrder = binary.BigEndian
	if *endian == "little" {
		order = binary.LittleEndian
	} else if *endian != "big" {
		log.Fatal("endian must be big or little")
	}

	stream := newHub()
	go serveBridgeHTTP(*httpAddr, stream, schemaJSON(fields, frameSize, *endian), dashboardHTML(fields, frameSize, *endian))

	conn, err := net.ListenPacket("udp", *listen)
	if err != nil {
		log.Fatal(err)
	}
	defer conn.Close()

	buf := make([]byte, 64*1024)
	for {
		n, _, err := conn.ReadFrom(buf)
		if err != nil {
			log.Fatal(err)
		}
		if n%frameSize != 0 {
			log.Printf("drop datagram size %d: not multiple of frame size %d", n, frameSize)
			continue
		}
		for off := 0; off < n; off += frameSize {
			line := []byte(
				`{"topic":` + jsonQuote(*topic) +
					`,"receive_time":` + strconv.FormatInt(time.Now().UnixNano(), 10) +
					`,"message":` + decodeFrame(fields, order, buf[off:off+frameSize]) +
					`}`,
			)
			fmt.Println(string(line))
			stream.publish(line)
		}
	}
}
