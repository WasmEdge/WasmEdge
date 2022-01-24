#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

export PATH="/toolchain/bin:$PATH"
export CC=gcc
export CXX=g++
export CPPFLAGS=-I/toolchain/include
export LDFLAGS=-L/toolchain/lib64
curl -s -L -O --remote-name-all https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.bz2
echo "f0397ba6e982c4450f27bf32a2a83292aba035b827a5623a14636ea583318c41  boost_1_76_0.tar.bz2" | sha256sum -c
bzip2 -dc boost_1_76_0.tar.bz2 | tar -xf -
# openssl configure need newer perl
curl -s -L -O --remote-name-all https://www.cpan.org/src/5.0/perl-5.34.0.tar.gz
tar -xf perl-5.34.0.tar.gz
cd perl-5.34.0
mkdir localperl
./Configure -des -Dprefix=$(pwd)/localperl/
make -j
# too long!
# make test
make install
cd ..
curl -s -L -O --remote-name-all https://www.openssl.org/source/openssl-1.1.1m.tar.gz
echo "f89199be8b23ca45fc7cb9f1d8d3ee67312318286ad030f5316aca6462db6c96 openssl-1.1.1m.tar.gz" | sha256sum -c
tar -xf openssl-1.1.1m.tar.gz
cd ./openssl-1.1.1m
mkdir openssl
$(pwd)/../perl-5.34.0/localperl/bin/perl ./config --prefix=$(pwd)/openssl --openssldir=$(pwd)/openssl
make -j
make test
make install
cd ..
if ! cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_PACKAGE="TGZ;TBZ2;TXZ;TZST;RPM;DEB" -DBoost_NO_SYSTEM_PATHS=TRUE -DBOOST_INCLUDEDIR=$(pwd)/boost_1_76_0/ -DOPENSSL_ROOT_DIR=$(pwd)/openssl-1.1.1m/openssl; then
    echo === CMakeOutput.log ===
    cat build/CMakeFiles/CMakeOutput.log
    echo === CMakeError.log ===
    cat build/CMakeFiles/CMakeError.log
    exit 1
fi
cmake --build build
cmake --build build --target package
