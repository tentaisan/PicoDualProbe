#!/bin/sh
# Run a JTAG configuration from the repository root.
# Set PICODUALPROBE_SERIAL to select a probe if more than one is connected.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"
exec openocd -f "${1:-scripts/openocd/neorv32-validate.cfg}"
