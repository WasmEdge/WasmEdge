#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2024 Second State INC
set -e

if [[ ! -v "${OPENVINO_VERSION}" ]]; then
  OPENVINO_VERSION="2025.0.0"
fi
if [[ ! -v "${OPENVINO_YEAR}" ]]; then
  OPENVINO_YEAR="2025"
fi

echo "Installing OpenVINO with version ${OPENVINO_VERSION}"
KEY_FILE=GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
wget https://apt.repos.intel.com/intel-gpg-keys/$KEY_FILE && \
    apt-key add $KEY_FILE && \
    rm -f $KEY_FILE
UBUNTU_VERSION="ubuntu${OPENVINO_UBUNTU_VERSION:-20}"
echo "deb https://apt.repos.intel.com/openvino/$OPENVINO_YEAR ${UBUNTU_VERSION} main" | tee /etc/apt/sources.list.d/intel-openvino-$OPENVINO_YEAR.list
apt update
apt-get -y install openvino-$OPENVINO_VERSION
ldconfig
