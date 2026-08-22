#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

# Static LLVM links need libzstd.a. Homebrew zstd may ship only the shared
# library, so build the static archive into the Homebrew prefix when missing.

set -euo pipefail

ZSTD_VERSION=1.5.7
ZSTD_SHA256=37d7284556b20954e56e1ca85b80226768902e2edabd3b649e9e72c0c9012ee3

ZSTD_PREFIX="$(brew --prefix zstd)"
if [ -f "${ZSTD_PREFIX}/lib/libzstd.a" ]; then
  echo "Using Homebrew static zstd: ${ZSTD_PREFIX}/lib/libzstd.a"
  exit 0
fi

echo "Homebrew zstd has no libzstd.a; building zstd ${ZSTD_VERSION}"
BREW_ZSTD_VERSION="$(brew list --versions zstd | awk '{print $2}')"
if [ "${BREW_ZSTD_VERSION%%_*}" != "${ZSTD_VERSION}" ]; then
  echo "::warning::Homebrew zstd ${BREW_ZSTD_VERSION} headers differ from the static zstd ${ZSTD_VERSION} archive"
fi
ZSTD_SOURCE="$(mktemp -d)"
trap 'rm -rf "${ZSTD_SOURCE}"' EXIT
curl -L --fail --retry 3 \
  "https://github.com/facebook/zstd/archive/refs/tags/v${ZSTD_VERSION}.tar.gz" \
  -o "${ZSTD_SOURCE}/zstd.tar.gz"
echo "${ZSTD_SHA256}  ${ZSTD_SOURCE}/zstd.tar.gz" | shasum -a 256 -c -
tar -xzf "${ZSTD_SOURCE}/zstd.tar.gz" -C "${ZSTD_SOURCE}"
cmake -S "${ZSTD_SOURCE}/zstd-${ZSTD_VERSION}/build/cmake" -B "${ZSTD_SOURCE}/build" -GNinja \
  -DZSTD_BUILD_STATIC=ON -DZSTD_BUILD_SHARED=OFF -DZSTD_BUILD_PROGRAMS=OFF
cmake --build "${ZSTD_SOURCE}/build" --target libzstd_static
cp "${ZSTD_SOURCE}/build/lib/libzstd.a" "${ZSTD_PREFIX}/lib/libzstd.a"
test -f "${ZSTD_PREFIX}/lib/libzstd.a"
