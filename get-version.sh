#!/bin/sh
SOURCE_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if command -v git >/dev/null 2>&1
then
    VERSION=$(git -C "$SOURCE_DIR" describe --exact-match --tags 2>/dev/null ||
              git -C "$SOURCE_DIR" rev-parse --short HEAD 2>/dev/null ||
              printf '%s' unknown)
else
    VERSION="unknown"
fi

if [ ! -e generated/probe ]
then
    mkdir -p generated/probe
fi

cat > generated/probe/version.h << EOF
#ifndef _PROBE_VERSION_H
#define _PROBE_VERSION_H

#define PROBE_VERSION "${VERSION}"

#endif
EOF
