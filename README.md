# Embedded Sensor Stack

A multi-process **C++17** sensor stack for embedded Linux, packaged as a **Buildroot** external tree and bootable in **QEMU**. No hardware needed.

```
sensord ──(Unix datagram socket)──▶ procd ──(state.json)──▶ metricsd ──HTTP──▶ curl / browser
 sensors + HAL                     moving average,            /health /metrics /state
                                   threshold alerts,
                                   IPC latency stats
```

| Daemon | Role |
|---|---|
| `sensord` | Reads simulated sensors through a HAL (`ISensor`) and sends readings to `procd` |
| `procd` | Moving average, threshold alerts (syslog), IPC latency tracking, atomic state file |
| `metricsd` | HTTP server: `/health`, `/metrics` (from `/proc`), `/state` (sensor JSON) |

## Build & run (native)

```bash
sudo apt install build-essential cmake curl
cmake -B build && cmake --build build -j
tests/smoke.sh build          # end-to-end test

# or run manually in 3 terminals
build/procd
build/sensord --interval-ms 200
build/metricsd --port 8080 && curl localhost:8080/state
```

## Cross-compile (ARM64)

```bash
sudo apt install g++-aarch64-linux-gnu
cmake -B build-arm64 -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-toolchain.cmake
cmake --build build-arm64 -j
```

## Buildroot + QEMU

This repo is a Buildroot *external tree*.

```bash
git clone https://github.com/buildroot/buildroot.git && cd buildroot
git checkout 2024.02.x            # any recent stable branch
make BR2_EXTERNAL=/path/to/embedded-sensor-stack qemu_aarch64_virt_defconfig
make menuconfig                   # External options -> sensor-stack (also enable C++ in toolchain)
make -j$(nproc)                   # 30-60 min first time, ~10 GB disk
./output/images/start-qemu.sh --serial-only
```

Log in as `root`, then:

```bash
curl localhost:8080/state
logread | grep -E 'sensord|procd|metricsd'
```

Forward the port to your host by adding `-nic user,hostfwd=tcp::8080-:8080` to the QEMU command in `start-qemu.sh`.

Init: BusyBox init uses `S50sensor-stack` (supervisor loop that restarts crashed daemons). If you select systemd in Buildroot, the `systemd/*.service` units are installed instead (`Restart=always`).

## Ideas to extend

- Replace `SimulatedSensor` with a real I2C driver (`/dev/i2c-*`)
- Benchmark Unix sockets vs POSIX message queues vs shared memory
- JSON config file, A/B OTA update agent, Yocto layer version


## Linux device-driver extension (optional)

The `drivers/virtual_sensor/` directory contains a software-only Linux misc character driver exposing `/dev/sensor0`. It demonstrates kernel-module and userspace interaction; it is **not** a physical I2C/SPI driver. Existing simulated sensors remain the default. To use the device as the temperature source, build/load the module on a matching Linux kernel and start `sensord` with `--device /dev/sensor0`. See [`docs/DRIVER_UPGRADE.md`](docs/DRIVER_UPGRADE.md) for build, load, test, limitations, and Buildroot notes.
