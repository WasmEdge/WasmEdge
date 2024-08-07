#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2024 Second State INC

set -e
echo "Installing OpenVINO with version 2024.2.0"
wget https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
echo "deb https://apt.repos.intel.com/openvino/2024 ubuntu20 main" | tee /etc/apt/sources.list.d/intel-openvino-2024.list
apt update
apt-get -y install openvino-2024.2.0
ldconfig
