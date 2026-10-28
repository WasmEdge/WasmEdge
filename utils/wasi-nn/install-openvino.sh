#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors
set -e

: "${OPENVINO_VERSION:=2025.0.0}"
: "${OPENVINO_YEAR:=${OPENVINO_VERSION%%.*}}"

echo "Installing OpenVINO with version ${OPENVINO_VERSION}"
if command -v apt-get >/dev/null 2>&1; then
  KEY_FILE=GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
  wget "https://apt.repos.intel.com/intel-gpg-keys/${KEY_FILE}"
  apt-key add "${KEY_FILE}"
  rm -f "${KEY_FILE}"
  UBUNTU_VERSION="ubuntu${OPENVINO_UBUNTU_VERSION:-20}"
  echo "deb https://apt.repos.intel.com/openvino/${OPENVINO_YEAR} ${UBUNTU_VERSION} main" | tee "/etc/apt/sources.list.d/intel-openvino-${OPENVINO_YEAR}.list"
  apt update
  apt-get -y install "openvino-${OPENVINO_VERSION}"
elif command -v yum >/dev/null 2>&1; then
  cat > /etc/yum.repos.d/intel-openvino.repo <<'EOF'
[intel-openvino]
name=Intel(R) Distribution of OpenVINO
baseurl=https://yum.repos.intel.com/openvino
enabled=1
gpgcheck=1
repo_gpgcheck=1
gpgkey=https://yum.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
EOF
  yum install -y "openvino-${OPENVINO_VERSION}"
else
  echo "OpenVINO installation requires apt-get or yum" >&2
  exit 1
fi
ldconfig
