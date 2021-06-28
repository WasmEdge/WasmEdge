#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -x

ROOT="$(realpath $(dirname "${BASH_SOURCE[0]}")/../../)"
API_FILE="$ROOT/thirdparty/wasi/api.hpp"
cargo build --release
curl -s -L --remote-name-all https://raw.githubusercontent.com/WebAssembly/WASI/master/phases/snapshot/witx/typenames.witx
patch -p1 < remove-resource.patch
patch -p1 < add-wasi_sock.patch
target/release/wasi-cpp-header generate --output "$API_FILE" typenames.witx
clang-format-12 -i "$API_FILE"
patch -p1 -d "$ROOT/thirdparty/wasi" < change-tag-type.patch
