# Arc Portable Pack

Target-neutral serialization smoke example. It uses `arc::pack` and target traits only, so it should stay valid for both the default ESP32-S3 path and future ESP32-S31 builds.

Run:

```sh
cd examples/portable/pack
. ../../../env.sh
idf.py build flash monitor
```
