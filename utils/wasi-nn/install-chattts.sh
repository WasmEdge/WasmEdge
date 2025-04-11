#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

set -e

apt-get update
apt-get -y install python3 python3-dev python3-pip

PACKAGES=('chattts==0.2.3')

version_ge() {
    [[ "$(printf '%s\n' "$1" "$2" | sort --version-sort | head -n 1)" == "$2" ]]
}

# --break-system-packages is implemented in 23.0.1
# https://pip.pypa.io/en/stable/news/#v23-0-1

PIP_VERSION=$(pip --version | awk '{ print $2 }')

if version_ge "${PIP_VERSION}" '23.0.1'; then
    pip install --break-system-packages "${PACKAGES[@]}"
else
    pip install "${PACKAGES[@]}"
fi
