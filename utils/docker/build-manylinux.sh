#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

export PATH="/toolchain/bin:$PATH"
export CC=gcc
export CXX=g++

cd
curl -s -L -O --remote-name-all https://dl.bintray.com/boostorg/release/1.75.0/source/boost_1_75_0.tar.bz2
echo 953db31e016db7bb207f11432bef7df100516eeb746843fa0486a222e3fd49cb boost_1_75_0.tar.bz2 | sha256sum -c
bzip2 -dc boost_1_75_0.tar.bz2 | tar -xf -
cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_PACKAGE="TGZ;TBZ2;TXZ;TZST;RPM" -DBOOST_INCLUDEDIR=$(pwd)/boost_1_75_0/ /wasmedge
cmake --build build
cmake --build build --target package
cp -v build/WasmEdge-*.tar.gz /wasmedge/WasmEdge.tar.gz
cp -v build/WasmEdge-*.tar.bz2 /wasmedge/WasmEdge.tar.bz2
cp -v build/WasmEdge-*.tar.xz /wasmedge/WasmEdge.tar.xz
cp -v build/WasmEdge-*.tar.zst /wasmedge/WasmEdge.tar.zst
cp -v build/WasmEdge-*.rpm /wasmedge/WasmEdge.rpm
