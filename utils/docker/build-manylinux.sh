#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cd
curl -s -L -O --remote-name-all https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.bz2
echo f0397ba6e982c4450f27bf32a2a83292aba035b827a5623a14636ea583318c41 boost_1_76_0.tar.bz2 | sha256sum -c
bzip2 -dc boost_1_76_0.tar.bz2 | tar -xf -
cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_PACKAGE="TGZ;TBZ2;TXZ;TZST;RPM" -DBOOST_INCLUDEDIR=$(pwd)/boost_1_76_0/ /wasmedge
cmake --build build
cmake --build build --target package
cp -v build/WasmEdge-*.tar.gz /wasmedge/WasmEdge.tar.gz
cp -v build/WasmEdge-*.tar.bz2 /wasmedge/WasmEdge.tar.bz2
cp -v build/WasmEdge-*.tar.xz /wasmedge/WasmEdge.tar.xz
cp -v build/WasmEdge-*.tar.zst /wasmedge/WasmEdge.tar.zst
cp -v build/WasmEdge-*.rpm /wasmedge/WasmEdge.rpm
