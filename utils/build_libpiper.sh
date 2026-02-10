#!/bin/bash
set -e

echo "::group::Build and install libpiper"
rm -rf piper-source

git clone https://github.com/OHF-Voice/piper1-gpl piper-source
cd piper-source/libpiper
git checkout 32b95f8c1f0dc0ce27a6acd1143de331f61af777
cmake -Bbuild-deps \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PWD/install" \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON

cmake --build build-deps
cmake --install build-deps
echo "::endgroup::"
