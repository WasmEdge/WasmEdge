#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2023 Second State INC

wget -O opencv.zip https://github.com/opencv/opencv/archive/4.x.zip

unzip opencv.zip

mkdir -p opencv-4.x/build && cd opencv-4.x/build
# Configure
cmake ..
# Build
cmake --build . -j2
# Install to system
cmake --install .
