#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

set -e
echo "Installing Python library!"
apt update
apt install -y python3-dev python3-pip

echo "Installing Neural Speed!"
wget https://raw.githubusercontent.com/intel/neural-speed/main/requirements.txt
pip install -r requirements.txt
pip install neural-speed==${NEURALSPEED_VERSION}
