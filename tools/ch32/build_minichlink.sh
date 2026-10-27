#!/bin/sh
# Build a pinned upstream minichlink with the PicoDualProbe backend.
set -eu
revision=84799bb3ae73cf7c7e54a7126dbb21b12de6eab2
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build=${1:-"$root/build-ch32-minichlink"}
mkdir -p "$build"
build=$(CDPATH= cd -- "$build" && pwd)
if [ ! -d "$build/ch32fun/.git" ]; then
    git clone --no-checkout https://github.com/cnlohr/ch32fun.git "$build/ch32fun"
fi
git -C "$build/ch32fun" fetch origin "$revision"
git -C "$build/ch32fun" checkout --detach "$revision"
# Apply local fixes once; fail if the pinned source has unexpected changes.
for patch in minichlink-zero-backend.patch minichlink-gdb-step.patch minichlink-register-preserve.patch; do
    if git -C "$build/ch32fun" apply --reverse --check "$root/tools/ch32/$patch" 2>/dev/null; then
        :
    else
        git -C "$build/ch32fun" apply --check "$root/tools/ch32/$patch"
        git -C "$build/ch32fun" apply "$root/tools/ch32/$patch"
    fi
done
make -C "$build/ch32fun/minichlink" minichlink C_S="minichlink.c pgm-wch-linke.c pgm-wch-isp.c pgm-esp32s2-ch32xx.c nhc-link042.c ardulink.c serial_dev.c pgm-b003fun.c minichgdb.c chips.c ch5xx.c $root/tools/ch32/minichlink_zero.c"
printf 'Built %s\n' "$build/ch32fun/minichlink/minichlink"
