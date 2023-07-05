#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2023 Second State INC

wget -O opencv.zip https://github.com/opencv/opencv/archive/4.x.zip

unzip opencv.zip

mkdir -p build && cd build
# Configure
cmake ../opencv-4.x
# Build
cmake --build . -j
# Install to system
sudo cmake --install .
