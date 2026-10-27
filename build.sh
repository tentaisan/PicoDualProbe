#!/bin/sh
# Build current firmware: ./build.sh [pico|zero] [clean]
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
target=${1:-pico}
case "$target" in
  zero) build_dir="$root/build-zero"; board=waveshare_rp2040_zero; option=DEBUG_ON_ZERO; output=debugprobe_on_zero.uf2 ;;
  pico) build_dir="$root/build"; board=pico; option=DEBUG_ON_PICO; output=debugprobe_on_pico.uf2 ;;
  *) printf 'Usage: %s [pico|zero] [clean]\n' "$0" >&2; exit 1 ;;
esac
case "${2:-}" in
  clean) rm -rf "$build_dir" ;;
  '') ;;
  *) printf 'Usage: %s [pico|zero] [clean]\n' "$0" >&2; exit 1 ;;
esac
: "${PICO_SDK_PATH:?Set PICO_SDK_PATH to a Pico SDK 2.x checkout}"
cmake -S "$root" -B "$build_dir" -DPICO_BOARD="$board" -D"$option"=ON
cmake --build "$build_dir" -j "${BUILD_JOBS:-2}"
printf 'Firmware: %s/%s\n' "$build_dir" "$output"
