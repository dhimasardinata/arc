# Net Client Example

This is a standalone ESP-IDF project under `examples/esp32s3/net_client`.

- It uses `arc::net::Uri` to split an HTTPS URL without heap allocation.
- It uses `arc::net::Http::https(...)` to configure an ESP-IDF HTTPS client without performing a request.
- It exercises the URI-based `arc::net::Tcp` and `arc::net::Tls` rejection path for unsafe percent-decoded hosts.
- It does not join Wi-Fi, open sockets, or contact the network.

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

- `build/arc_net_client.bin`
- `build/arc_net_client.elf`

## API Shape

```cpp
const auto uri = arc::net::Uri::parse("https://example.com/api?unit=ph");
const auto endpoint = arc::net::Uri::endpoint(*uri, 443);
const auto client = arc::net::Http::https("https://example.com/api?unit=ph");
```

Use the URI helpers when an application wants to validate and split a URL once, then keep the reconnect, Wi-Fi, certificate, and request policy explicit.
