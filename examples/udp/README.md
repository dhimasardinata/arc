# UDP Example

This is the right first network example for Arc, but it should stay opt-in.

- Core 1 stays deterministic and never blocks on Wi-Fi.
- Core 1 pushes telemetry through `arc::Ring`.
- Core 0 owns NVS, Wi-Fi station mode, and the UDP socket.
- Core 0 sends latest-wins control back to Core 1 through `arc::Reg`.

Why UDP first:

- it matches Arc's drop-on-full telemetry policy
- it keeps the transport stateless and cheap
- it proves the asymmetric dual-core split without dragging MQTT into the baseline

Recommended shape:

```text
examples/udp
├── README.md
└── main
    ├── app.hpp
    ├── app_main.cpp
    └── net.hpp
```

Recommended Core 0 flow:

1. `nvs_flash_init()`
2. Wi-Fi station init
3. socket setup with `sendto()`
4. drain `ctx.tx`
5. update `ctx.ctl` or other `arc::Reg` words with latest commands

The root app stays transport-free on purpose. That keeps the default template small, deterministic, and readable.
