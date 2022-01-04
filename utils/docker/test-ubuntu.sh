#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

export CC=gcc
export CXX=g++
curl -s -L -O --remote-name-all https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.bz2
echo "f0397ba6e982c4450f27bf32a2a83292aba035b827a5623a14636ea583318c41  boost_1_76_0.tar.bz2" | sha256sum -c
bzip2 -dc boost_1_76_0.tar.bz2 | tar -xf -
if ! cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=Debug -DWASMEDGE_BUILD_TESTS=ON -DBoost_NO_SYSTEM_PATHS=TRUE -DBOOST_INCLUDEDIR=$(pwd)/boost_1_76_0/; then
    echo === CMakeOutput.log ===
    cat build/CMakeFiles/CMakeOutput.log
    echo === CMakeError.log ===
    cat build/CMakeFiles/CMakeError.log
    exit 1
fi
cmake --build build
export LD_LIBRARY_PATH="$(pwd)/build/lib/api:$LD_LIBRARY_PATH"
cd build
ctest --output-on-failure
cd -
