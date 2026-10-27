#!/bin/sh
# Host-only tests; do not require a probe or the Pico SDK.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
${CC:-cc} -std=c11 -DDEBUG_ON_ZERO -I"$root/tests/stubs" -I"$root/src" \
    "$root/tests/test_usb_endpoint.c" -o "$tmp/test_usb_endpoint"
"$tmp/test_usb_endpoint"
${CC:-cc} -std=c11 -I"$root/src" "$root/tests/test_ch32_transport.c" \
    "$root/src/ch32_transport.c" -o "$tmp/test_ch32_transport"
"$tmp/test_ch32_transport"
${CC:-cc} -std=c11 -I"$root/tests/stubs" -I"$root/src" "$root/tests/test_swo_trace.c" \
    "$root/src/swo_trace.c" -o "$tmp/test_swo_trace"
"$tmp/test_swo_trace"
${CC:-cc} -std=c11 -DDEBUG_ON_ZERO -I"$root/tests/stubs" -I"$root/src" \
    "$root/tests/test_ch32_vendor.c" "$root/CMSIS_DAP/CMSIS/DAP/Firmware/Source/DAP_vendor.c" -o "$tmp/test_ch32_vendor"
"$tmp/test_ch32_vendor"
${CC:-cc} -std=c11 -I"$root/src" "$root/tests/test_zero_settings.c" \
    "$root/src/zero_settings.c" -o "$tmp/test_zero_settings"
"$tmp/test_zero_settings"
${CC:-cc} -std=c11 -DDEBUG_ON_ZERO -I"$root/src" "$root/tests/test_zero_config_cmd.c" \
    "$root/src/zero_config_cmd.c" "$root/src/zero_settings.c" -o "$tmp/test_zero_config_cmd"
"$tmp/test_zero_config_cmd"
python3 "$root/tests/test_zero_config_cli.py"
