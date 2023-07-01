#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

if [[ ! -v "${OPENVINO_VERSION}" ]]; then
  OPENVINO_VERSION="2023.0.0"
fi
if [[ ! -v "${OPENVINO_YEAR}" ]]; then
  OPENVINO_YEAR="2023"
fi

set -e
echo "Installing OpenVINO with version ${OPENVINO_VERSION}"
wget https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
sudo apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
echo "deb https://apt.repos.intel.com/openvino/2023 ubuntu22 main" | sudo tee /etc/apt/sources.list.d/intel-openvino-2023.list
sudo apt update
apt-cache search openvino
sudo apt install openvino
source /opt/intel/openvino_2021/bin/setupvars.sh
ldconfig
