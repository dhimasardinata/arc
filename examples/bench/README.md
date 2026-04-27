# Arc ESP32-S3 Benchmark

On-device benchmark for Arc hot primitives on a real ESP32-S3.

It measures runtime on the target using the Xtensa cycle counter and prints cycles/op for:

- `Spsc` single and batch transfers
- `Mpsc` and `DenseMpsc`
- `Fanin` single and batch transfers
- `SeqReg`
- `Stream` write, frame encode, and frame read
- MQTT, WebSocket, and CoAP caller-buffer codecs
- DSP dot/scale/mac/FIR/peak kernels
- standard `memcpy` and Arc async-DMA copy on the same 256-byte payload
- hardware RNG
- accelerated SHA-256
- AES ECB, CBC, CTR, OFB, and GCM encryption paths

It intentionally does not publish fake numbers for SPI/I2C/UART/ADC/LCD/CAN/etc. Those need a board fixture, attached devices, and a stable wiring policy to be meaningful.

Run:

```sh
cd examples/bench
. ../../env.sh
idf.py build flash monitor
```

Look for `arc-bench` log lines. Each line prints total operations, cycles per operation, and nanoseconds per operation for the real ESP32-S3 run.
