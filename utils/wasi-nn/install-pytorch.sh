#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors
set -e

if [[ ! -n ${PYTORCH_VERSION} ]]; then
  PYTORCH_VERSION="2.12.1"
fi

PYTORCH_DEFAULT_DIR="libtorch-cxx11abi"
PYTORCH_SHA="78b99d296e0557742c780266a3fb94824ec382e8a285f4d8edf92e033572c5d4"

for i in "$@"; do
  case $i in
  --disable-cxx11-abi)
    # The pre-cxx11-abi build is only published up to torch 2.5.1.
    # This mode is kept for the branches still building with the pre-cxx11
    # ABI; remove it after the 0.18.0 release.
    PYTORCH_VERSION="2.5.1"
    PYTORCH_DEFAULT_DIR="libtorch"
    PYTORCH_SHA="21d05ad61935fc70912c779443dba112bda9c9ec1c999345d724935828f81c55"
    shift
    ;;
  esac
done

if [[ ! -n ${PYTORCH_INSTALL_TO} ]]; then
  PYTORCH_INSTALL_TO=${PYTORCH_DEFAULT_DIR}
fi

if [ ! -d ${PYTORCH_INSTALL_TO} ]; then
  curl -sSf -L -O --remote-name-all https://download.pytorch.org/libtorch/cpu/libtorch-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip
  echo "${PYTORCH_SHA} libtorch-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip" | sha256sum -c
  PYTORCH_UNPACK_DIR=$(mktemp -d -p $(dirname ${PYTORCH_INSTALL_TO}))
  unzip -q "libtorch-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip" -d ${PYTORCH_UNPACK_DIR}
  mv ${PYTORCH_UNPACK_DIR}/libtorch ${PYTORCH_INSTALL_TO}
  rmdir ${PYTORCH_UNPACK_DIR}
  rm -f "libtorch-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip"
fi
