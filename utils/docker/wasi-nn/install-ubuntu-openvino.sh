#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

set -e
echo "Installing OpenVINO with version ${OPENVINO_VERSION}"
curl -sSL https://apt.repos.intel.com/openvino/$OPENVINO_YEAR/GPG-PUB-KEY-INTEL-OPENVINO-$OPENVINO_YEAR | gpg --dearmor >/usr/share/keyrings/GPG-PUB-KEY-INTEL-OPENVINO-$OPENVINO_YEAR.gpg
echo "deb [signed-by=/usr/share/keyrings/GPG-PUB-KEY-INTEL-OPENVINO-$OPENVINO_YEAR.gpg] https://apt.repos.intel.com/openvino/$OPENVINO_YEAR all main" | tee /etc/apt/sources.list.d/intel-openvino-$OPENVINO_YEAR.list
apt update
apt install -y intel-openvino-runtime-ubuntu20-$OPENVINO_VERSION
