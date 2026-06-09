#!/bin/sh

# Copyright 2026, UNSW
# SPDX-License-Identifier: BSD-2-Clause

set -e

if [ "$#" -ne 2 ]; then
    echo "usage: hello_x86.sh /path/to/lionsos /path/to/microkit/sdk"
    exit 1
fi

LIONSOS=$1
MICROKIT_SDK=$2

MICROKIT_BOARD=x86_64_generic_vtx
MICROKIT_CONFIG=debug

echo "CI|INFO: building hello_x86 for board: ${MICROKIT_BOARD}"

BUILD_DIR=$LIONSOS/ci_build/hello_x86/${MICROKIT_BOARD}/${MICROKIT_CONFIG}

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

export MICROKIT_SDK
export MICROKIT_CONFIG
export MICROKIT_BOARD
export BUILD_DIR

cd "$LIONSOS/examples/hello_x86"
make

echo "CI|INFO: Passed hello_x86 build test"
