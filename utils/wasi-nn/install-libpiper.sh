#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

set -e

git clone https://github.com/OHF-Voice/piper1-gpl.git piper-source
cd piper-source/libpiper

cmake -Bbuild \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DONNXRUNTIME_DIR=/usr/local \

cmake --build build
cmake --install build

cd ../..
rm -rf piper-source
