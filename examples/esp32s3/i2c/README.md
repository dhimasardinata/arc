# I2C Example

This is a standalone ESP-IDF project under `examples/esp32s3/i2c`.

- It uses `arc::I2cBus` for recoverable bus startup.
- It uses `arc::I2c` for compile-time device address and clock binding.
- It probes an EEPROM-style address and attempts one register read without panicking if the device is absent.

## API Shape

```cpp
using Bus = arc::I2cBus<0, 8, 9>;
using Dev = arc::I2c<Bus, 0x50, 400'000>;

ESP_ERROR_CHECK(Bus::init());
auto seen = Bus::probe(0x50);
auto err = Dev::xfer(tx, tx_bytes, rx, rx_bytes);
```
