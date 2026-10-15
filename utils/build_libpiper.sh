#!/bin/bash
set -e

echo "::group::Build and install libpiper"
rm -rf piper-source

git clone https://github.com/OHF-Voice/piper1-gpl piper-source
cd piper-source/libpiper
git checkout 639388b6317fc4731e91d53da42aea68fd4166ff
cmake -Bbuild-deps \
  -DCMAKE_BUILD_TYPE=Release \
  -DPIPER_BUILD_EXE=OFF \
  -DCMAKE_INSTALL_PREFIX="$PWD/install" \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON

cmake --build build-deps
cmake --install build-deps
echo "::endgroup::"
