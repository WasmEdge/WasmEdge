#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2024 Second State INC

if [[ ! -n ${PYTORCH_VERSION} ]]; then
  PYTORCH_VERSION="2.5.1"
fi

if [[ ! -n ${PYTORCH_INSTALL_TO} ]]; then
  PYTORCH_INSTALL_TO=.
fi

PYTORCH_LINK="libtorch-cxx11-abi"
PYTORCH_SHA="618ca54eef82a1dca46ff1993d5807d9c0deb0bae147da4974166a147cb562fa"

for i in "$@"; do
  case $i in
  --disable-cxx11-abi)
    PYTORCH_LINK="libtorch"
    PYTORCH_SHA="21d05ad61935fc70912c779443dba112bda9c9ec1c999345d724935828f81c55"
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
