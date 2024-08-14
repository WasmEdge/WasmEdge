#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

set -x

ROOT="$(realpath $(dirname "${BASH_SOURCE[0]}")/../../)"
API_FILE="$ROOT/thirdparty/wasi/api.hpp"
cargo build --release
curl -s -L --remote-name-all https://raw.githubusercontent.com/WebAssembly/WASI/main/legacy/preview1/witx/typenames.witx
patch -p1 < add-wasi_sock.patch
patch -p1 < add-addrinfo.patch
patch -p1 < add-wasi_opt.patch
target/release/wasi-cpp-header generate --output "$API_FILE" typenames.witx
clang-format-12 -i "$API_FILE"
patch -p1 -d "$ROOT/thirdparty/wasi" < change-tag-type.patch
