#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -x

WD="$(realpath $(dirname "${BASH_SOURCE[0]}"))"
ROOT="$(realpath $WD/../../)"
WITX_DIR="$ROOT/doc/witx"

cd $WD
cargo build --release
target/release/witx-doc "$WITX_DIR"
