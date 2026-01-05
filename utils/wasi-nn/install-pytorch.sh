#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2024 Second State INC

if [[ ! -n ${PYTORCH_VERSION} ]]; then
  PYTORCH_VERSION="2.4.0"
fi

if [[ ! -n ${PYTORCH_INSTALL_TO} ]]; then
  PYTORCH_INSTALL_TO=.
fi

# SHA checksum for PyTorch 2.4.0 (cxx11 ABI)
PYTORCH_SHA="9d16cc0da41e057f20c0be5f26d7418f969e857631cfcb86550ccdecfee8de60"
PYTORCH_LINK="libtorch-cxx11-abi"

for i in "$@"; do
  case $i in
  --disable-cxx11-abi)
    PYTORCH_LINK="libtorch"
    # If using non-ABI 2.4.0, we would need a different SHA here, but we default to ABI.
    shift
    ;;
  esac
done

if [ ! -d ${PYTORCH_INSTALL_TO}/libtorch ]; then
  curl -s -L -O --remote-name-all https://download.pytorch.org/libtorch/cpu/${PYTORCH_LINK}-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip
  echo "${PYTORCH_SHA} ${PYTORCH_LINK}-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip" | sha256sum -c
  unzip -q "${PYTORCH_LINK}-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip" -d ${PYTORCH_INSTALL_TO}
  rm -f "${PYTORCH_LINK}-shared-with-deps-${PYTORCH_VERSION}%2Bcpu.zip"
fi
