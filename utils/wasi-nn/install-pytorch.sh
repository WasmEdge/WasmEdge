#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2024 Second State INC

if [[ ! -n ${PYTORCH_VERSION} ]]; then
  PYTORCH_VERSION="2.4.1"
fi

if [[ ! -n ${PYTORCH_INSTALL_TO} ]]; then
  PYTORCH_INSTALL_TO=.
fi

PYTORCH_LINK="libtorch-cxx11-abi"
PYTORCH_SHA="415c3ed51c766a6ef20dc10b2e60fae7f10a3ae8aa62223d6f4bccc1fc98740b"

for i in "$@"; do
  case $i in
  --disable-cxx11-abi)
    PYTORCH_LINK="libtorch"
    PYTORCH_SHA="f49d55df661c566c29a7a75bcae2fad69177eaebd330618d42ca162eb3a1fad1"
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
