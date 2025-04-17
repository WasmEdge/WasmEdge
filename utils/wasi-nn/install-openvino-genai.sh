#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2024 Second State INC
set -e

if [[ ! -v "${OPENVINOGEN_VERSION}" ]]; then
  OPENVINOGEN_VERSION="2025.0.0.0"
fi
if [[ ! -v "${OPENVINOGEN_YEAR}" ]]; then
  OPENVINOGEN_YEAR="2025.0"
fi
if [[ ! -v "${OPENVINOGEN_DIRNAME}" ]]; then
  OPENVINOGEN_DIRNAME="openvino_genai"
fi

if [[ ! -v "${OPENVINO_UBUNTU_VERSION}" ]]; then
  source /etc/os-release
  OPENVINO_UBUNTU_VERSION="${VERSION_ID::2}"
fi

UBUNTU_VERSION="ubuntu${OPENVINO_UBUNTU_VERSION:-20}"
OPENVINOGEN_TGZ_NAME="openvino_genai_${UBUNTU_VERSION}_${OPENVINOGEN_VERSION}_x86_64"


echo "Installing OpenVINO GenAI with version ${OPENVINOGEN_VERSION}"
curl -L https://storage.openvinotoolkit.org/repositories/openvino_genai/packages/${OPENVINOGEN_YEAR}/linux/${OPENVINOGEN_TGZ_NAME}.tar.gz --output ${OPENVINOGEN_TGZ_NAME}.tgz
tar -xf ${OPENVINOGEN_TGZ_NAME}.tgz
mv ${OPENVINOGEN_TGZ_NAME} $OPENVINOGEN_DIRNAME
./${OPENVINOGEN_DIRNAME}/install_dependencies/install_openvino_dependencies.sh -y

rm ${OPENVINOGEN_TGZ_NAME}.tgz

echo "OpenVINO GenAI installed at `pwd`/$OPENVINOGEN_DIRNAME"
echo "Please source the setupvars.sh script in the OpenVINO GenAI directory to use the OpenVINO GenAI tools."
echo "# source $PWD/$OPENVINOGEN_DIRNAME/setupvars.sh"
