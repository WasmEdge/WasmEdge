#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2026 Second State INC

set -e

PIPER_REPO="https://github.com/OHF-Voice/piper1-gpl.git"
PIPER_COMMIT="32b95f8c1f0dc0ce27a6acd1143de331f61af777"
PIPER_INSTALL_TO="/usr/local"

case "$(uname -m)" in
  'x86_64') ;;
  'aarch64') ;;
  *)
    echo "Unsupported architecture for libpiper: $(uname -m)" >&2
    exit 1
    ;;
esac

git clone --depth 1 "${PIPER_REPO}" piper-source
cd piper-source
git fetch --depth 1 origin "${PIPER_COMMIT}"
git checkout FETCH_HEAD

cd libpiper

cmake -Bbuild -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${PIPER_INSTALL_TO}"
cmake --build build
cmake --install build

cd ../..
rm -rf piper-source

ldconfig
