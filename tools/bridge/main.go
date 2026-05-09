package main

import (
	"crypto/sha1"
	"encoding/base64"
	"encoding/binary"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"log"
	"math"
	"net"
	"net/http"
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

func decodeFrame(fields []field, order binary.ByteOrder, frame []byte) map[string]any {
	out := make(map[string]any, len(fields))
	pos := 0
	for _, f := range fields {
		switch f.kind {
		case "u8":
			out[f.name] = frame[pos]
		case "i8":
			out[f.name] = int8(frame[pos])
		case "u16":
			out[f.name] = order.Uint16(frame[pos:])
		case "i16":
			out[f.name] = int16(order.Uint16(frame[pos:]))
		case "u32":
			out[f.name] = order.Uint32(frame[pos:])
		case "i32":
			out[f.name] = int32(order.Uint32(frame[pos:]))
		case "u64":
			out[f.name] = order.Uint64(frame[pos:])
		case "i64":
			out[f.name] = int64(order.Uint64(frame[pos:]))
		case "f32":
			out[f.name] = math.Float32frombits(order.Uint32(frame[pos:]))
		case "f64":
			out[f.name] = math.Float64frombits(order.Uint64(frame[pos:]))
		}
		pos += f.size
	}
	return out
}

func websocketAccept(key string) string {
	const guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
	sum := sha1.Sum([]byte(key + guid))
	return base64.StdEncoding.EncodeToString(sum[:])
}

func writeFrame(w io.Writer, payload []byte) error {
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
	if _, err := w.Write(header); err != nil {
		return err
	}
	_, err := w.Write(payload)
	return err
}

func serveWS(h *hub) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if !strings.EqualFold(r.Header.Get("Upgrade"), "websocket") {
			http.Error(w, "websocket upgrade required", http.StatusUpgradeRequired)
			return
		}
		key := r.Header.Get("Sec-WebSocket-Key")
		if key == "" {
			http.Error(w, "missing websocket key", http.StatusBadRequest)
			return
		}
		hijacker, ok := w.(http.Hijacker)
		if !ok {
			http.Error(w, "hijack unsupported", http.StatusInternalServerError)
			return
		}
		conn, bufrw, err := hijacker.Hijack()
		if err != nil {
			return
		}
		defer conn.Close()
		fmt.Fprintf(bufrw, "HTTP/1.1 101 Switching Protocols\r\n")
		fmt.Fprintf(bufrw, "Upgrade: websocket\r\n")
		fmt.Fprintf(bufrw, "Connection: Upgrade\r\n")
		fmt.Fprintf(bufrw, "Sec-WebSocket-Accept: %s\r\n\r\n", websocketAccept(key))
		if err := bufrw.Flush(); err != nil {
			return
		}
		ch := h.add()
		defer h.remove(ch)
		for msg := range ch {
			if err := writeFrame(conn, msg); err != nil {
				return
			}
		}
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
	http.HandleFunc("/ws", serveWS(stream))
	http.HandleFunc("/", func(w http.ResponseWriter, _ *http.Request) {
		fmt.Fprintf(w, "arc bridge websocket: /ws\n")
	})
	go func() {
		log.Fatal(http.ListenAndServe(*httpAddr, nil))
	}()

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
			msg := map[string]any{
				"topic":        *topic,
				"receive_time": strconv.FormatInt(time.Now().UnixNano(), 10),
				"message":      decodeFrame(fields, order, buf[off:off+frameSize]),
			}
			line, err := json.Marshal(msg)
			if err != nil {
				log.Print(err)
				continue
			}
			fmt.Println(string(line))
			stream.publish(line)
		}
	}
}
