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

CMAKE_BUILD_TYPE="Release"
WASMEDGE_PLUGIN_WASI_CRYPTO="OFF"

for i in "$@"; do
  case $i in
    -DCMAKE_BUILD_TYPE=*)
      CMAKE_BUILD_TYPE="${i#*=}"
      shift
      ;;
    -DWASMEDGE_PLUGIN_WASI_CRYPTO=*)
      WASMEDGE_PLUGIN_WASI_CRYPTO=$(echo ${i#*=} | tr '[:lower:]' '[:upper:]')
      shift
      ;;
    *)
      ;;
  esac
done

CMAKE_OPTS="-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
if [ ${WASMEDGE_PLUGIN_WASI_CRYPTO} == "ON" ]; then
  echo "Building wasi-crypto..."
  # install openssl
  curl -s -L -O --remote-name-all https://www.openssl.org/source/openssl-1.1.1n.tar.gz
  echo "40dceb51a4f6a5275bde0e6bf20ef4b91bfc32ed57c0552e2e8e15463372b17a openssl-1.1.1n.tar.gz" | sha256sum -c
  tar -xf openssl-1.1.1n.tar.gz
  cd ./openssl-1.1.1n
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
  export PATH="$(pwd)/localperl/bin/:$PATH"
  cd ..
  # configure by previous perl
  mkdir openssl
  ./perl-5.34.0/localperl/bin/perl ./config --prefix=$(pwd)/openssl --openssldir=$(pwd)/openssl
  make -j
  make test
  make install
  cd ..
  CMAKE_OPTS="${CMAKE_OPTS} -DWASMEDGE_PLUGIN_WASI_CRYPTO=ON -DOPENSSL_ROOT_DIR=$(pwd)/openssl-1.1.1n/openssl"
fi

if ! cmake -Bbuild -GNinja ${CMAKE_OPTS} -DWASMEDGE_BUILD_PACKAGE="TGZ;TBZ2;TXZ;TZST;RPM;DEB" -DBoost_NO_SYSTEM_PATHS=TRUE -DBOOST_INCLUDEDIR=$(pwd)/boost_1_79_0/; then
    echo === CMakeOutput.log ===
    cat build/CMakeFiles/CMakeOutput.log
    echo === CMakeError.log ===
    cat build/CMakeFiles/CMakeError.log
    exit 1
fi
cmake --build build
cmake --build build --target package
