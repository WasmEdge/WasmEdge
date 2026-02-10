#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2026 Second State INC

set -e

SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
PATCH_PATH="${SCRIPT_DIR}/libpiper.patch"

if [ ! -f "${PATCH_PATH}" ]; then
    echo "Error: libpiper.patch not found at ${PATCH_PATH}"
    exit 1
fi

PIPER_REPO="https://github.com/OHF-Voice/piper1-gpl.git"
PIPER_COMMIT="32b95f8c1f0dc0ce27a6acd1143de331f61af777"
PIPER_INSTALL_TO="/usr/local"

case "$(uname -m)" in
  'x86_64') ;;
  'aarch64') ;;
  *)
    echo "Unsupported architecture for libpiper: $(uname -m)" >&2
    exit 1
    ;;
esac

rm -rf piper-source
git clone --depth 1 "${PIPER_REPO}" piper-source
cd piper-source
git fetch --depth 1 origin "${PIPER_COMMIT}"
git checkout FETCH_HEAD

cp "${PATCH_PATH}" .
patch -p1 < libpiper.patch
cd libpiper

cmake -Bbuild \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${PIPER_INSTALL_TO}" \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON
cmake --build build --parallel $(nproc)
cmake --install build

echo "Copying espeak-ng static libraries to ${PIPER_INSTALL_TO}/lib..."

if [ -f "build/espeak_ng-install/lib/libespeak-ng.a" ]; then
    cp "build/espeak_ng-install/lib/libespeak-ng.a" "${PIPER_INSTALL_TO}/lib/"
else
    find build -name "libespeak-ng.a" -exec cp {} "${PIPER_INSTALL_TO}/lib/" \; -quit
fi

find build -name "libucd.a" -exec cp {} "${PIPER_INSTALL_TO}/lib/" \; -quit

if [ ! -f "${PIPER_INSTALL_TO}/lib/libespeak-ng.a" ]; then
    echo "Error: Failed to install libespeak-ng.a"
    exit 1
fi

if [ ! -f "${PIPER_INSTALL_TO}/lib/libucd.a" ]; then
    echo "Error: Failed to install libucd.a"
    exit 1
fi

echo "Installing espeak-ng-data to ${PIPER_INSTALL_TO}/share..."

if [ -d "build/espeak_ng-install/share/espeak-ng-data" ]; then
    mkdir -p "${PIPER_INSTALL_TO}/share"
    cp -r "build/espeak_ng-install/share/espeak-ng-data" "${PIPER_INSTALL_TO}/share/"
    echo "Espeak-ng-data installed successfully."
else
    echo "Error: espeak-ng-data directory not found in build tree!"
    exit 1
fi

cd ../..
rm -rf piper-source

ldconfig
