#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

export PATH="/toolchain/bin:$PATH"
export CC=gcc
export CXX=g++
export CPPFLAGS=-I/toolchain/include
export LDFLAGS=-L/toolchain/lib64
curl -s -L -O --remote-name-all https://boostorg.jfrog.io/artifactory/main/release/1.79.0/source/boost_1_79_0.tar.bz2
echo "475d589d51a7f8b3ba2ba4eda022b170e562ca3b760ee922c146b6c65856ef39  boost_1_79_0.tar.bz2" | sha256sum -c
git config --global --add safe.directory $(pwd)
bzip2 -dc boost_1_79_0.tar.bz2 | tar -xf -
if ! cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_PACKAGE="TGZ;TBZ2;TXZ;TZST;RPM;DEB" -DBoost_NO_SYSTEM_PATHS=TRUE -DBOOST_INCLUDEDIR=$(pwd)/boost_1_79_0/; then
    echo === CMakeOutput.log ===
    cat build/CMakeFiles/CMakeOutput.log
    echo === CMakeError.log ===
    cat build/CMakeFiles/CMakeError.log
    exit 1
fi
cmake --build build
cmake --build build --target package
