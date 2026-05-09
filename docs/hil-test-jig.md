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

## Pass Criteria

- `arc::I2cBus::init()` returns a recoverable error and can be retried after the
  fault clears.
- SPI transactions fail without wedging the RTOS or corrupting the next transfer.
- CAN/TWAI priority spam is recorded but does not starve the control loop.
- ESP-NOW loss makes remote state stale, then `DeadReckoning` predicts until the
  next valid scheduled frame arrives.
