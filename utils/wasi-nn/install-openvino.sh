#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

if [[ ! -v "${OPENVINO_VERSION}" ]]; then
  OPENVINO_VERSION="2023.0.1"
fi
if [[ ! -v "${OPENVINO_YEAR}" ]]; then
  OPENVINO_YEAR="2023"
fi

if [["${OPENVINO_YEAR}" -eq "2021"]]; then
set -e
echo "Installing OpenVINO with version ${OPENVINO_VERSION}"
curl -sSL https://apt.repos.intel.com/openvino/$OPENVINO_YEAR/GPG-PUB-KEY-INTEL-OPENVINO-$OPENVINO_YEAR | gpg --dearmor > /usr/share/keyrings/GPG-PUB-KEY-INTEL-OPENVINO-$OPENVINO_YEAR.gpg
echo "deb [signed-by=/usr/share/keyrings/GPG-PUB-KEY-INTEL-OPENVINO-$OPENVINO_YEAR.gpg] https://apt.repos.intel.com/openvino/$OPENVINO_YEAR all main" | tee /etc/apt/sources.list.d/intel-openvino-$OPENVINO_YEAR.list
apt update
apt install -y intel-openvino-runtime-ubuntu20-$OPENVINO_VERSION
fi

if [["${OPENVINO_YEAR}" -eq "2023"]]; then
set -e
echo "Installing OpenVINO with version ${OPENVINO_VERSION}"
curl -sSL https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB | apt-key add
echo "deb https://apt.repos.intel.com/openvino/$OPENVINO_YEAR all main" | sudo tee /etc/apt/sources.list.d/intel-openvino-$OPENVINO_YEAR.list
apt update
apt install -y openvino-$OPENVINO_VERSION
fi
source /opt/intel/openvino_2021/bin/setupvars.sh
ldconfig
