#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2024 Second State INC
OPENCV_VERSION=${OPENCV_VERSION:-4.8.0}

wget -O opencv.zip https://github.com/opencv/opencv/archive/refs/tags/${OPENCV_VERSION}.zip

unzip opencv.zip
mv opencv-${OPENCV_VERSION} opencv

mkdir -p opencv/build && cd opencv/build
# Configure
cmake -GNinja ..
# Build
cmake --build .
# Install to system
cmake --install .
