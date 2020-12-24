#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

export PATH="/toolchain/bin:$PATH"
export CC=gcc
export CXX=g++

cd
curl -s -L -O --remote-name-all https://dl.bintray.com/boostorg/release/1.75.0/source/boost_1_75_0.tar.bz2
echo 953db31e016db7bb207f11432bef7df100516eeb746843fa0486a222e3fd49cb boost_1_75_0.tar.bz2 | sha256sum -c
bzip2 -dc boost_1_75_0.tar.bz2 | tar -xf -
cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_PACKAGE="TGZ;TBZ2;TXZ;TZST;RPM" -DBOOST_INCLUDEDIR=$(pwd)/boost_1_75_0/ /ssvm
cmake --build build
cmake --build build --target package
cp -v build/SSVM-*.tar.gz /ssvm/SSVM.tar.gz
cp -v build/SSVM-*.tar.bz2 /ssvm/SSVM.tar.bz2
cp -v build/SSVM-*.tar.xz /ssvm/SSVM.tar.xz
cp -v build/SSVM-*.tar.zst /ssvm/SSVM.tar.zst
cp -v build/SSVM-*.rpm /ssvm/SSVM.rpm
