# Linux character-driver extension

## Scope and honesty
This adds a software-only Linux misc character driver named `/dev/sensor0`. It demonstrates a kernel module, character-device registration, `read()` handling, and a C++ userspace consumer. It does **not** implement a physical sensor bus driver; the sample is generated in the kernel using `jiffies`. Existing simulated-sensor mode remains the default.

## Build the existing userspace stack
```sh
cmake -S . -B build
cmake --build build -j
tests/smoke.sh build
```

## Build the kernel module on a matching Linux kernel
Install headers matching the running kernel, then:
```sh
cd drivers/virtual_sensor
make KDIR=/lib/modules/$(uname -r)/build
```
Kernel modules must be built for the exact target kernel/configuration. Git Bash on Windows cannot load Linux modules. WSL kernels may require matching WSL kernel headers; a Linux VM or configured QEMU kernel is often easier.

## Load and test (Linux only)
```sh
sudo insmod sensor_driver.ko
ls -l /dev/sensor0
cat /dev/sensor0
g++ -std=c++17 -Wall -Wextra userspace/sensor_reader.cpp -o build/sensor_reader
./build/sensor_reader
sudo rmmod sensor_driver
dmesg | tail
```

## Use it with sensord
Run the stack as usual, but start sensor daemon with the device option:
```sh
build/procd
build/metricsd --port 8080
build/sensord --interval-ms 500 --device /dev/sensor0
```
The temperature channel comes from `/dev/sensor0`; humidity remains simulated. Without `--device`, both original simulated sensors are used.

## Kernel/userspace boundary
The driver returns one ASCII floating-point sample per read. `LinuxDeviceSensor` reads and parses it. `sensord` converts it to the existing `Reading` IPC structure; `procd` and `metricsd` remain unchanged.

## Limitations / next steps
- This teaching driver is read-only and emits synthetic values.
- For real hardware, implement an I2C/SPI client driver using the relevant kernel subsystem and device-tree binding; do not treat this misc driver as a hardware bus driver.
- For production, add locking/state design, robust error handling, permissions, ABI/versioning, and automated kernel-module tests.
- Buildroot/QEMU integration requires compiling/installing the module against the exact Buildroot kernel. The current Buildroot package continues to build/install the original userspace daemons; the module is built separately until kernel-version integration is configured.
