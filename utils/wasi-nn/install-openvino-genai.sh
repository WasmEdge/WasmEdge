#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2024 Second State INC
set -e

if [[ ! -v "${OPENVINOGEN_VERSION}" ]]; then
  OPENVINOGEN_VERSION="2025.0.0.0"
fi
if [[ ! -v "${OPENVINOGEN_YEAR}" ]]; then
  OPENVINO_VERSION="2025.0"
fi

UBUNTU_VERSION="ubuntu${OPENVINO_UBUNTU_VERSION:-20}"
OPENVINOGEN_TGZ_NAME="openvino_genai_${OPENVINOGEN_YEAR}_${OPENVINOGEN_VERSION}_x86_64"


echo "Installing OpenVINO GenAI with version ${OPENVINOGEN_VERSION}"
curl -L https://storage.openvinotoolkit.org/repositories/openvino_genai/packages/${OPENVINO_VERSION}/linux/${OPENVINOGEN_TGZ_NAME}.tar.gz --output ${OPENVINOGEN_TGZ_NAME}.tgz
tar -xf ${OPENVINOGEN_TGZ_NAME}.tgz
./${OPENVINOGEN_TGZ_NAME}/install_dependencies/install_openvino_dependencies.sh -y
source ./${OPENVINOGEN_TGZ_NAME}/setupvars.sh
