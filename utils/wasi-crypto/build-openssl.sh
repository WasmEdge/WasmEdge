#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2024 Second State INC

echo "Building OpenSSL for wasi-crypto..."
# Get OpenSSL source
curl -s -L -O --remote-name-all https://www.openssl.org/source/openssl-1.1.1n.tar.gz
echo "40dceb51a4f6a5275bde0e6bf20ef4b91bfc32ed57c0552e2e8e15463372b17a openssl-1.1.1n.tar.gz" | sha256sum -c
tar -xf openssl-1.1.1n.tar.gz
cd ./openssl-1.1.1n
# OpenSSL configure need newer perl
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
# Configure by previous perl
mkdir openssl
./perl-5.34.0/localperl/bin/perl ./config --prefix=$(pwd)/openssl --openssldir=$(pwd)/openssl
make -j
make test
make install
cd ..
