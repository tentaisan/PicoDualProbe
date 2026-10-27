#!/bin/sh
# Compile from an empty temporary build directory; never touches existing builds.
set -eu
: "${PICO_SDK_PATH:?Set PICO_SDK_PATH to a Pico SDK 2.x checkout}"
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cmake -S "$root" -B "$tmp/build" -DPICO_BOARD=waveshare_rp2040_zero -DDEBUG_ON_ZERO=ON
cmake --build "$tmp/build" -j "${BUILD_JOBS:-2}"
test -s "$tmp/build/debugprobe_on_zero.uf2"
printf '%s\n' 'Clean Zero build passed'
