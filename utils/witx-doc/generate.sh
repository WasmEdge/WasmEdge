#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

set -x

WD="$(realpath $(dirname "${BASH_SOURCE[0]}"))"
ROOT="$(realpath $WD/../../)"
WITX_DIR="$ROOT/docs/witx"

cd $WD
cargo build --release
target/release/witx-doc "$WITX_DIR"
