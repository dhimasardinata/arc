# Copy Example

This is a standalone ESP-IDF project under `examples/copy`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::Copy` to offload memory movement to the ESP32-S3 async DMA memcpy driver.
- It uses `arc::dmabuf` so source and destination buffers are explicitly DMA-capable.
- It can use raw `arc::Cache` calls, but the example now shows `send_coherent(...)` plus `finish_coherent(...)` so the DMA path stays explicit without repeating ownership handoff boilerplate.

## What It Shows

At runtime, this example repeatedly:

- fills a DMA-capable source buffer
- launches a DMA copy into a DMA-capable destination buffer through the cache-coherent non-blocking path
- computes the source checksum while DMA owns the destination
- observes the ISR completion counter
- verifies the copy by checksum

The CPU is not copying the 4096-byte payload byte-by-byte. It only submits the transfer and observes completion.

## Build And Run

From this directory:

```bash
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

Binary outputs:

- `build/arc_copy.bin`
- `build/arc_copy.elf`

## API Shape

The program surface stays small:

```cpp
using Dma = arc::Copy<8, 64, 0, arc::CopyBackend::ahb>;

Dma::boot();
Dma::Ticket move{};
Dma::send_coherent(move, dst.view(), src.view());
auto src_sum = sum(src.view());
Dma::finish_coherent(move);
```

The main API pieces are:

- `arc::Copy<Backlog, BurstBytes, Weight, Backend>`
- `arc::Copy::boot()`
- `arc::Copy::send(dst, src, bytes)`
- `arc::Copy::send_coherent(ticket, dst, src)`
- `arc::Copy::finish_coherent(ticket)`
- `arc::Copy::ready(ticket)`
- `arc::Copy::copy(dst, src, bytes)`
- `arc::Copy::copy_coherent(dst, src)`
- `arc::Copy::copy_coherent_strict(dst, src)`
- `arc::Copy::sent()`
- `arc::Copy::done()`
- `arc::Copy::bytes()`
- `arc::Copy::idle()`
- `arc::CopyBackend::auto_dma`
- `arc::CopyBackend::ahb`
