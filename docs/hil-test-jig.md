# HIL Test Jig

This is the hardware test shape for faults that host tests cannot prove.

## Nodes

- Node A: device under test running Arc firmware and reporting `arc::HilDeadline`
  results.
- Node B: physical fault injector that can pull I2C low, drop SPI CS during a
  transaction, and flood TWAI/CAN with priority-inversion frames.
- Node C: ESP-NOW jammer that transmits during scheduled swarm windows to test
  `arc::net::DistributedRcu` and `DeadReckoning` recovery.

## Script Model

Use `arc::HilScript<N>` to describe which node applies each `arc::HilFault`, at
which tick, and for how long. Board policy implements `Policy::apply(HilAction)`.

For nanosecond-scale closed-loop tests, use `arc::hil::DigitalTwin` on a
simulator S3. The capture policy samples DUT PWM timing, the twin steps a fixed
`arc::dsp::StateSpace` model, and the encoder policy emits ABI/Z-style feedback
through board-owned RMT/GPIO wiring.

## Pass Criteria

- `arc::I2cBus::init()` returns a recoverable error and can be retried after the
  fault clears.
- SPI transactions fail without wedging the RTOS or corrupting the next transfer.
- CAN/TWAI priority spam is recorded but does not starve the control loop.
- ESP-NOW loss makes remote state stale, then `DeadReckoning` predicts until the
  next valid scheduled frame arrives.

## Evidence Artifact

The jig should emit one JSON object per line. Each required case must report
`"status":"pass"`:

```json
{"case":"i2c_fault_recovery","status":"pass","node":"A"}
{"case":"spi_fault_recovery","status":"pass","node":"A"}
{"case":"can_priority_spam","status":"pass","node":"A"}
{"case":"espnow_stale_recover","status":"pass","node":"A"}
```

Validate the captured artifact before attaching it to a release or safety review:

```bash
./tools/hil-evidence-check.py hil.jsonl
./tools/hil-evidence-check.py --format json hil.jsonl
```
