#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2023 Second State INC

wget -O opencv.zip https://github.com/opencv/opencv/archive/refs/tags/4.8.0.zip

unzip opencv.zip
mv opencv-4.8.0 opencv

mkdir -p opencv/build && cd opencv/build
# Configure
cmake -GNinja ..
# Build
cmake --build .
# Install to system
cmake --install .
