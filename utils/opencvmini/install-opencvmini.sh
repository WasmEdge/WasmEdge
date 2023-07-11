#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2023 Second State INC

wget -O opencv.zip https://github.com/opencv/opencv/archive/4.x.zip
wget -O opencv_contrib.zip https://github.com/opencv/opencv_contrib/archive/4.x.zip

unzip opencv.zip
unzip opencv_contrib.zip
mv opencv-4.x opencv
mv opencv_contrib-4.x opencv_contrib

mkdir -p opencv/build && cd opencv/build
# Configure
cmake -GNinja \
  -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
  ..
# Build
cmake --build .
# Install to system
cmake --install .
