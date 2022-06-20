#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

source /opt/intel/openvino_2021/bin/setupvars.sh
ldconfig
git config --global --add safe.directory $(pwd)
apt update
apt install -y gcovr
if ! cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=Debug -DWASMEDGE_BUILD_TESTS=ON -DWASMEDGE_BUILD_COVERAGE=ON -DWASMEDGE_WASINN_BUILD_OPENVINO=ON .; then
    echo === CMakeOutput.log ===
    cat build/CMakeFiles/CMakeOutput.log
    echo === CMakeError.log ===
    cat build/CMakeFiles/CMakeError.log
    exit 1
fi
cmake --build build
LD_LIBRARY_PATH=$(pwd)/build/lib/api cmake --build build --target codecov
