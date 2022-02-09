#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -x

ROOT="$(realpath $(dirname "${BASH_SOURCE[0]}")/../../)"
cargo build --release

for name in "asymmetric_common" "common" "external_secrets" "kx" "signatures" "symmetric" ;
    do 
        curl -s -L --remote-name-all https://raw.githubusercontent.com/WebAssembly/wasi-crypto/8e883a5a173062e1ecf7dbd7124188ab8046425d/witx/proposal_${name}.witx
        # target/release/wasi-cpp-header generate --output "$ROOT/thirdparty/wasi/crypto/${name}.hpp" proposal_${name}.witx
    done

target/release/wasi-cpp-header generate --output $ROOT/thirdparty/wasi/crypto/api.hpp proposal_kx.witx proposal_asymmetric_common.witx  proposal_common.witx proposal_signatures.witx proposal_symmetric.witx proposal_external_secrets.witx
clang-format-12 -i "$ROOT/thirdparty/wasi/crypto/$api.hpp"
