#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -x

ROOT="$(realpath $(dirname "${BASH_SOURCE[0]}")/../../)"
cargo build --release

for name in "asymmetric_common" "common" "external_secrets" "kx" "signatures" "symmetric" ;
    do 
        curl -s -L --remote-name-all https://raw.githubusercontent.com/WebAssembly/wasi-crypto/main/witx/proposal_${name}.witx
    done

target/release/wasi-cpp-header generate --output $ROOT/thirdparty/wasi_crypto/api.hpp proposal_kx.witx proposal_asymmetric_common.witx  proposal_common.witx proposal_signatures.witx proposal_symmetric.witx proposal_external_secrets.witx
clang-format-12 -i "$ROOT/thirdparty/wasi_crypto/$api.hpp"
patch -p1 -d "$ROOT/thirdparty/wasi_crypto" < crypto-custom.patch
