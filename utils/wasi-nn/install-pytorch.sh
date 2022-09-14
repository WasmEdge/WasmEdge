#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

if [[ ! -n ${PYTORCH_VERSION} ]]; then
  PYTORCH_VERSION="1.8.2"
fi

<<<<<<< HEAD
if [[ ! -n ${PYTORCH_INSTALL_TO} ]]; then
  PYTORCH_INSTALL_TO=.
fi

PYTORCH_LINK="libtorch-cxx11-abi"
PYTORCH_SHA="b76d6dd4380e2233ce6f7654e672e13aae7c871231d223a4267ef018dcbfb616"

for i in "$@"; do
  case $i in
    --disable-cxx11-abi)
      PYTORCH_LINK="libtorch"
      PYTORCH_SHA="b5ddadc9addc054d8503f4086546f0cbcfdc3fc70087863bbd7b0e3300e3247f"
      shift
      ;;
  esac
done

if [ ! -d ${PYTORCH_INSTALL_TO}/libtorch ]; then
    curl -s -L -O --remote-name-all https://download.pytorch.org/libtorch/lts/1.8/cpu/${PYTORCH_LINK}-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip
    echo "${PYTORCH_SHA} ${PYTORCH_LINK}-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip" | sha256sum -c
    unzip -q "${PYTORCH_LINK}-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip" -d ${PYTORCH_INSTALL_TO}
    rm -f "${PYTORCH_LINK}-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip"
=======
if [[ $# -lt 1 ]]; then
  DOWNLOAD_TO=.
else
  DOWNLOAD_TO=$1
fi

if [ ! -d $DOWNLOAD_TO/libtorch ]; then
    curl -s -L -O --remote-name-all https://download.pytorch.org/libtorch/lts/1.8/cpu/libtorch-cxx11-abi-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip
    echo "b76d6dd4380e2233ce6f7654e672e13aae7c871231d223a4267ef018dcbfb616 libtorch-cxx11-abi-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip" | sha256sum -c
    unzip -q "libtorch-cxx11-abi-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip" -d $DOWNLOAD_TO
    rm -f "libtorch-cxx11-abi-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip"
>>>>>>> af9e22d8 ([WASI] Refine the WASI-NN pytorch scripts)
fi
