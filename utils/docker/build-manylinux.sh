#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

export PATH="/toolchain/bin:$PATH"
export CC=gcc
export CXX=g++

cd
curl -s -L -O --remote-name-all https://dl.bintray.com/boostorg/release/1.75.0/source/boost_1_75_0.tar.bz2
bzip2 -dc boost_1_75_0.tar.bz2 | tar -xf -
cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_PACKAGE=RPM -DBOOST_INCLUDEDIR=$(pwd)/boost_1_75_0/ /ssvm
cmake --build build
cmake --build build --target package
cp -v build/SSVM-*.rpm /ssvm/SSVM-manylinux1.rpm
