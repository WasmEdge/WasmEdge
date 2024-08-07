#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2024 Second State INC

if [[ ! -v "${CMAKE_BUILD_TYPE}" ]]; then
  CMAKE_BUILD_TYPE=Release
fi

ldconfig
git config --global --add safe.directory $(pwd)
if ! cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DWASMEDGE_BUILD_TESTS=ON -DWASMEDGE_PLUGIN_WASI_NN_BACKEND="OpenVINO" .; then
    echo === CMakeOutput.log ===
    cat build/CMakeFiles/CMakeOutput.log
    echo === CMakeError.log ===
    cat build/CMakeFiles/CMakeError.log
    exit 1
fi
cmake --build build
