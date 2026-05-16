# Getting Started

Arc is for fixed ESP32-S3 firmware where hidden allocation, cache incoherency, or task migration would be a product bug.

## Environment

Use the repo environment wrapper so CMake target selection stays portable:

```sh
source ./env.sh
```

The wrapper defaults to `esp32s3` unless `ARC_TARGET` is set. Keep target changes in the Arc target selection path; do not bypass it with ad hoc ESP-IDF target commands.

## First Build

Build the root firmware project:

```sh
idf.py build
```

Build a focused ESP32-S3 example:

```sh
idf.py -C examples/esp32s3/udp build
```

## Fast Local Checks

For source changes that do not require flashing hardware, start with:

```sh
./tools/host-tests.sh
./tools/format.sh --check
./tools/check-repo.sh
```

For public headers or component dependency changes, validate the clangd compile database:

```sh
./tools/clangd-compile-commands.py --validate-arc-headers
```

## Project Shape

- Public headers live in `components/arc/include/arc/`.
- `components/arc/include/arc.hpp` is the umbrella include.
- Root firmware entry code lives in `main/app_main.cpp`.
- ESP32-S3 examples live in `examples/esp32s3/`.
- Host tests and compile contracts live in `tests/host/`.
- Repo automation lives in `tools/`.

The shortest path to understanding the design is `docs/architecture.md`, then the focused example closest to the hardware path you need.
