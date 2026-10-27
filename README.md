# PicoDualProbe

PicoDualProbe turns an RP2040 board into a USB debug probe. The RP2040-Zero
build combines CMSIS-DAP SWD and JTAG with a USB-to-UART bridge. It also
includes experimental WCH single-wire SWIO, two-wire RVSWD, and UART-format
SWO capture. The standard Raspberry Pi Pico build supports SWD and JTAG.

## Start here

1. Download the Zero or Pico artifact from a successful [GitHub Actions
   build](#github-actions-builds), or [build the firmware](#build-from-source).
2. Hold BOOTSEL while connecting the probe to USB, then copy the extracted UF2
   to the `RPI-RP2` drive.
3. Connect a target using the [Zero pinout](#pinout) and common ground. Use
   3.3 V logic; the tested CH32 connections also need a 1 kΩ pull-up.
4. Use OpenOCD for SWD or JTAG, or the supplied minichlink backend for CH32.
   The examples and target-specific wiring are below.

Pin assignments and LED behavior can be changed using the
[startup configuration](#zero-startup-pin-configuration). SWO is implemented
but has not yet been tested with a physical target.

## Tested targets and compatibility

| Target | Protocol | Status |
|--------|----------|--------|
| RP2040 (Pico) | ARM SWD | Both cores detected; memory and debug operations tested |
| ESP32 (NodeMCU-32S) | JTAG | Both TAP IDCODEs detected; CPU0 halt, register read and resume tested; CPU1 debug not verified |
| CH32V003 | WCH SWIO, one wire | Identification, flash and GDB operations tested |
| CH32V203K8T6 | WCH RVSWD, two wire | Identification, flash and GDB operations tested |
| NeoRV32 (Alchitry Pt V2) | JTAG | Configuration provided; original project reports testing, not retested on this Zero build |
| SAMD21 | ARM SWD | Configuration provided; not verified on this Zero build |
| Other ARM or RISC-V targets | SWD or JTAG | Compatibility depends on the target and debug tools; not generally verified |

## Quick Start

```bash
# 1. Download or build the Zero UF2, then copy it to RPI-RP2.
# 2. Wire it up:
#    SWD: GP1→SWCLK  GP2→SWDIO  GP8→nRESET  GND→GND
#    JTAG: GP1→TCK  GP3→TDI  GP6→TMS  GP7→TDO  GP8→nRESET
# 3. A ready LED, if fitted to GP12, lights when ready.

# SWD — ARM target:
openocd -f scripts/openocd/picodualprobe-swd.cfg -f target/rp2040.cfg

# JTAG — RISC-V target:
openocd -f scripts/openocd/picodualprobe-jtag.cfg -f scripts/openocd/neorv32.cfg
```

If multiple PicoDualProbe devices are attached, set `PICODUALPROBE_SERIAL` to
the intended probe's USB serial number before running the JTAG command. The
NEORV32 configurations identify a specific target; adapt the target settings
for other JTAG devices.

## Hardware smoke tests

Connect one target at a time, set `PICODUALPROBE_SERIAL` to the probe's USB
serial, and run `sh scripts/verify.sh esp32`, `ch32v203`, `ch32v003` or
`rp2040`. These checks identify the target; they do not test every debugger
operation. See the sections below for wiring and target-specific limits.

## Pinout

Default RP2040-Zero GPIO assignments:

```
GP1  = SWCLK / TCK   GP4 = UART TX → target RX
GP2  = SWDIO         GP5 = UART RX ← target TX
GP3  = TDI           GP6 = TMS
GP7  = TDO           GP8 = nRESET
GP9  = error LED     GP10 = JTAG LED
GP11 = SWD LED       GP12 = ready LED
GP13 = reserved LED  GND = target ground
```

The GPIO LEDs default to active-high. Pin assignments, LED polarity, enabled
LEDs and LED roles can be changed using the startup configuration below. The
error indication is reserved for future error reporting, and GP13 is currently
unused by default.

## ESP32 JTAG

On the tested NodeMCU-32S with an ESP32, use the board's 3.3 V logic levels:

| Zero | ESP32 | Signal |
|------|-------|--------|
| GP1 | GPIO13 | TCK |
| GP3 | GPIO12 | TDI |
| GP6 | GPIO14 | TMS |
| GP7 | GPIO15 | TDO |
| GND | GND | common ground |

Power the ESP32 from its own USB connection. GPIO12 is a boot strapping pin;
power up the ESP32 before connecting GP3, and avoid resetting or power cycling
it while an external adapter is driving that pin. With OpenOCD 0.12.0 the
following configures the tested CPU0 target at 100 kHz:

```bash
export PICODUALPROBE_SERIAL=YOUR_ZERO_SERIAL
openocd -f scripts/openocd/picodualprobe-jtag.cfg -c 'adapter speed 100' \
  -c 'set ESP32_ONLYCPU 1' -f target/esp32.cfg
```

On the tested ESP32, both TAPs returned IDCODE `0x120034e5`; CPU0 halt,
register read, and resume have been tested. CPU1 debugging is not verified.
JTAG availability and GPIO use depend on the target configuration.

## Zero startup pin configuration

The default pin layout below is used until a valid saved configuration is
present. `tools/zero_config.py` uses the Zero's CMSIS-DAP serial number and
requires PyUSB. Only one host should use the probe while configuring it.
Saving, rebooting, verifying and restoring settings were tested on a Zero;
power interruption during a write has only been simulated in host tests. The
final two flash sectors are reserved for alternating settings records; writes
are refused if the linked firmware overlaps them. Sequence numbers, CRC and
format version select the newest valid record, preserving the previous record if
a write is interrupted. A legacy single-sector record is read and migrated on
the next save. Empty or invalid storage falls back to defaults.

```sh
python3 tools/zero_config.py --serial YOUR_ZERO_SERIAL show
python3 tools/zero_config.py --serial YOUR_ZERO_SERIAL pinout
cp tools/zero-default.json my-zero.json
# Edit my-zero.json. Review and disconnect debug/trace clients first.
python3 tools/zero_config.py --serial YOUR_ZERO_SERIAL set my-zero.json
# Reboot Zero normally, then:
python3 tools/zero_config.py --serial YOUR_ZERO_SERIAL verify my-zero.json
python3 tools/zero_config.py --serial YOUR_ZERO_SERIAL led ready
```

`pinout` combines the current GPIO settings with the external header wiring in
`tools/zero-connector.json`. The included map is an illustrative example and does not describe a physical
connector. Edit a copy to match your wiring and use `pinout --map OTHER.json`.
The map can record LEDs, their GPIOs, colors, and wiring; their
configured roles, enable state, and polarity are shown alongside the connector.
LED physical order is not assumed. To record a wire color, replace a row entry
such as `"GP1"` with `{"pin": "GP1", "wire_color": "orange"}` in a copy of
that file. No wire colors are assumed in the default map. A wire color describes
the physical connection, so it stays with the connector position when signal
assignments change. Pending settings are shown separately. The physical map is a user
record and is not detected or validated against actual wiring.

`show` distinguishes active values from saved values, reports an empty or
corrupt saved record and indicates pending changes. `set` validates the GPIO
assignment and reads it back, but it does not change the active mapping until
reboot. `reset` saves the default configuration, also applied at reboot.
`verify` compares both the active and saved configuration with a JSON file;
it does not verify physical wiring. `led` lights one enabled LED temporarily.

`led_enabled` is a bitmask in error, JTAG, SWD, ready, reserved order; `31`
enables all five. `led_active_high` and `swo_enabled` are 0 or 1. Disabled LEDs
and disabled SWO do not reserve their configured GPIO. All enabled signal pins
must be distinct; GPIO16 is reserved for the onboard RGB LED. SWDIO must be
SWCLK + 1. CDC uses UART1 (TX: GP4/8/20/24; RX: GP5/9/21/25); SWO uses
UART0 RX (GP1/13/17/29). Some Zero GPIOs require soldering to board pads.
Only 3.3 V logic is supported. Settings are saved only while debug and trace are
inactive; flash modification briefly pauses both cores, so disconnect any
active serial session as well.

## Experimental SWO trace (RP2040-Zero)

The Zero build provides CMSIS-DAP SWO capture in asynchronous UART/NRZ mode.
Connect the target's SWO output to Zero **GP17** and use a common GND, in
addition to the target's normal SWD wiring. GP17 is a provisional assignment:
The Zero startup configuration selects a free UART0 RX GPIO. Pin conflicts
are rejected before saving. Target signals must use 3.3 V logic.

The implementation reserves RP2040 UART0 for SWO and keeps USB CDC on UART1.
Diagnostic stdio on UART0 is disabled in the Zero build. The maximum accepted
requested baud rate is 2 MHz, not a guarantee of lossless reception. An
internal 4096-byte buffer reports receive overrun and framing errors through
CMSIS-DAP status; Manchester mode and a separate streaming USB endpoint are
not implemented. OpenOCD can configure its CMSIS-DAP adapter for UART SWO
and a file or socket trace destination; the exact TPIU configuration, clock and
ITM setup depend on the target and firmware.

**Hardware validation is pending.** Host tests exercise command behavior and
the ring buffer, not the UART sampling, throughput, timing or interaction with
actual target trace. `tests/hardware/swo_source` and `check_swo_uart.py` provide
a RAM-only Pico UART waveform source and receiver check for a later hardware
test; they do not generate or validate ITM trace. Do not assume a particular baud rate is reliable until
measured on the target and wiring.

## Experimental CH32V003 SWIO

The Zero build includes an experimental single-wire WCH SWIO transport on GP2.
Connect GP2 to CH32V003 PD1/SWIO, common ground, and a 1 kΩ pull-up to the
3.3 V target supply. Power the target separately at 3.3 V. Disconnect SWD/JTAG
sessions before using SWIO. This is not standard CMSIS-DAP SWD.

On Linux, install `git`, Python 3, a C compiler, `make`, `libusb-1.0-0-dev` and
`libudev-dev`, then build the pinned upstream minichlink and the Zero backend:

```bash
./tools/ch32/build_minichlink.sh
MINICHLINK=build-ch32-minichlink/ch32fun/minichlink/minichlink
$MINICHLINK -C zero -l YOUR_ZERO_SERIAL -i
$MINICHLINK -C zero -l YOUR_ZERO_SERIAL -r backup.bin flash 16384 -e
```

USB access requires suitable permissions. The `-l` serial selects the probe;
if omitted, exactly one accessible matching probe must be present. To write a
binary and read it back, replace `SIZE` with the binary's byte count:

```bash
$MINICHLINK -C zero -l YOUR_ZERO_SERIAL -w firmware.bin flash -b
$MINICHLINK -C zero -l YOUR_ZERO_SERIAL -r readback.bin flash SIZE -e
cmp firmware.bin readback.bin
```

For GDB, start `$MINICHLINK -C zero -l YOUR_ZERO_SERIAL -G`, then in a
second terminal run `gdb-multiarch firmware.elf` and `target remote :3333`.
GDB flash breakpoints rewrite target flash; detach cleanly to remove them.
Keep a verified backup: interrupted debugger sessions may leave modified flash.

On a CH32V003, identification, flash read/write and GDB operations were
tested. The host build uses pinned upstream ch32fun/minichlink sources.

## Experimental CH32 two-wire RVSWD

The Zero firmware contains an experimental RVSWD implementation for CH32V203
and other two-wire QingKe targets.
Connect Zero GP1 to PA14/SWCLK, GP2 to PA13/SWDIO, and GND to target GND.
Connect a 1 kΩ pull-up from GP2/PA13 to 3.3 V. Power the target at 3.3 V and
hold BOOT0 low. Check the package and board pinout for the particular target. RVSWD is different from ARM SWD.

For initial read-only diagnostics after building the Zero firmware and the
minichlink backend above, use `python3 tools/ch32_swio_probe.py --rvswd --serial
YOUR_ZERO_SERIAL` (requires PyUSB). For the tested CH32V203K8T6, minichlink
reported 64 KB flash; read it twice and compare before writing:

```bash
$MINICHLINK -C zero203 -l YOUR_ZERO_SERIAL -i
$MINICHLINK -C zero203 -l YOUR_ZERO_SERIAL -r backup1.bin flash 65536 -e
$MINICHLINK -C zero203 -l YOUR_ZERO_SERIAL -r backup2.bin flash 65536 -e
cmp backup1.bin backup2.bin
```

Use `-C zero203` in place of `-C zero` in the write and GDB commands above.
Do not write flash until identification, readout and backup succeed. The generic
upstream minichlink CH32V203 memory map is not a guarantee of the flash size
for every part or configuration; check the actual target and memory split.
On a CH32V203K8T6, identification, flash read/write and GDB operations were
tested. Other two-wire CH32 devices are unverified.

## GitHub Actions builds

GitHub Actions runs host tests and builds Zero and Pico UF2 files on pushes,
pull requests, and manual dispatch. Open a successful run under the repository's
**Actions** tab and download `debugprobe-on-zero` or `debugprobe-on-pico` from
its artifacts. GitHub packages each artifact as a ZIP; extract the UF2 before
copying it to the BOOTSEL drive. Artifact retention and download access depend
on repository settings. These artifacts are builds, not automatically published
GitHub Releases.

## Build From Source

Install CMake, a C/C++ build toolchain,
`arm-none-eabi-gcc`, Python 3, Git, and Raspberry Pi Pico SDK 2.x with its
TinyUSB submodule; this branch was built with Pico SDK 2.1.1. From the project
root, point `PICO_SDK_PATH` at that SDK checkout and run:

```bash
export PICO_SDK_PATH=/path/to/pico-sdk
git submodule update --init
cmake -S . -B build-zero -DPICO_BOARD=waveshare_rp2040_zero -DDEBUG_ON_ZERO=ON
cmake --build build-zero -j 4
```

The firmware is `build-zero/debugprobe_on_zero.uf2`. Before copying it to a
BOOTSEL drive, identify the intended probe, especially if more than one RP2040
board is attached.

## Hardware validation

SWD and UART were tested with RP2040, JTAG identification with ESP32, and
SWIO/RVSWD with CH32V003/CH32V203. SWO remains unverified on hardware. An
RP2040 core selection error was observed during reset after a UART test;
subsequent core detection succeeded. Power interruption during settings writes
has been simulated in host tests only.

## Host-only checks

Run `./tests/run.sh` (C compiler and Python 3 with PyUSB required) to exercise USB endpoint boundaries, queue backpressure,
reset handling and CH32 transport selection without hardware. These stubs do
not test USB electrical behavior or validate every CMSIS-DAP request. With
`PICO_SDK_PATH` set, `./tests/clean_build.sh` also builds a Zero image in a
new temporary directory and removes the directory on exit. The independent
CH32 host-tool build is described in the CH32 section above.

## Companion Projects

| Project | What | 
|---------|------|
| **[NeoRV32 on Alchitry Pt V2](https://github.com/greg676/vivado_neorv32)** | The RISC-V platform this probe was built to debug |
| **[NeoRV32 MCP Server](https://github.com/greg676/neorv32-mcp)** | AI agent bridge — 15 tools for hardware control |

## Credits

Built on the [Raspberry Pi Debug Probe](https://github.com/raspberrypi/debugprobe) firmware, with PicoDualProbe contributions by Greg Jackson and other contributors.

The project license is in [LICENSE](LICENSE); individual files and included
components may carry their own notices. The WCH transport notices are in
`src/CH32_SWIO_LICENSE.txt` and `src/CH32_RVSWD_LICENSE.txt`. Bundled CMSIS
files include Apache-2.0 notices and its license is provided at
[CMSIS_DAP/LICENSE-APACHE-2.0](CMSIS_DAP/LICENSE-APACHE-2.0). FreeRTOS and
the Pico SDK have their own license notices in their respective checkouts.
