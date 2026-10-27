#!/bin/sh
# Read-only hardware smoke tests. Connect one target at a time.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"
target=${1:-}
serial=${PICODUALPROBE_SERIAL:-}
case "$serial" in *[!a-zA-Z0-9_-]*) echo 'Invalid PICODUALPROBE_SERIAL' >&2; exit 2;; esac
case "$target" in
  esp32)
    # Connect TCK GP1, TDI GP3, TMS GP6, TDO GP7 and common GND.
    set -- -f scripts/openocd/picodualprobe-jtag.cfg
    if [ -n "$serial" ]; then set -- "$@" -c "adapter serial $serial"; fi
    log=$(mktemp)
    trap 'rm -f "$log"' EXIT HUP INT TERM
    if ! openocd "$@" -c 'adapter speed 100' \
      -c 'jtag newtap esp32 cpu0 -irlen 5 -expected-id 0x120034e5' \
      -c 'jtag newtap esp32 cpu1 -irlen 5 -expected-id 0x120034e5' \
      -c init -c scan_chain -c shutdown >"$log" 2>&1; then cat "$log" >&2; exit 1; fi
    cat "$log"
    if grep -Eq 'Error:|UNEXPECTED' "$log" || [ "$(grep -c 'tap/device found: 0x120034e5' "$log")" -ne 2 ]; then
      echo 'ESP32 JTAG chain validation failed' >&2; exit 1
    fi
    ;;
  rp2040)
    # Connect SWCLK GP1, SWDIO GP2 and common GND.
    set -- -f scripts/openocd/picodualprobe-swd.cfg
    if [ -n "$serial" ]; then set -- "$@" -c "adapter serial $serial"; fi
    log=$(mktemp)
    trap 'rm -f "$log"' EXIT HUP INT TERM
    if ! openocd "$@" -f target/rp2040.cfg -c init -c targets -c shutdown >"$log" 2>&1; then cat "$log" >&2; exit 1; fi
    cat "$log"
    if grep -q 'Error:' "$log" || [ "$(grep -c 'Cortex-M0+ r0p1 processor detected' "$log")" -lt 2 ]; then
      echo 'RP2040 SWD validation failed' >&2; exit 1
    fi
    ;;
  ch32v003|ch32v203)
    # GP2 is SWIO on V003; GP1/GP2 are clock/data on V203.
    set -- tools/ch32_swio_probe.py
    if [ "$target" = ch32v203 ]; then set -- "$@" --rvswd; fi
    if [ -n "$serial" ]; then set -- "$@" --serial "$serial"; fi
    if ! output=$(python3 "$@" 2>&1); then printf '%s\n' "$output" >&2; exit 1; fi
    printf '%s\n' "$output"
    if [ "$target" = ch32v003 ]; then prefix=003; else prefix=203; fi
    if ! printf '%s\n' "$output" | grep -Eq "WCH debug PART: 0x${prefix}[0-9a-fA-F]{5}"; then
      echo "Response does not match $target" >&2; exit 1
    fi
    ;;
  *)
    echo "Usage: PICODUALPROBE_SERIAL=<serial> sh scripts/verify.sh {esp32|rp2040|ch32v003|ch32v203}" >&2
    exit 2
    ;;
esac
