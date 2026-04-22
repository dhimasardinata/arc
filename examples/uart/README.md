# UART Example

This is a standalone ESP-IDF project under `examples/uart`.

- It uses `arc::Uart` for recoverable serial startup.
- It binds port, pins, baud, framing, and buffers at compile time.
- It writes one line and performs a bounded non-blocking read.

## API Shape

```cpp
using Serial = arc::Uart<UART_NUM_1, 17, 18>;

ESP_ERROR_CHECK(Serial::init());
auto wrote = Serial::write(data, bytes);
auto got = Serial::read(buffer, bytes, timeout_ms);
```
