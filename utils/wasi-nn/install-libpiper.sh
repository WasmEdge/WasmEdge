#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

set -e

SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
PATCH_PATH="${SCRIPT_DIR}/libpiper.patch"

if [ ! -f "${PATCH_PATH}" ]; then
    echo "Error: libpiper.patch not found at ${PATCH_PATH}"
    exit 1
fi

PIPER_REPO="https://github.com/OHF-Voice/piper1-gpl.git"
PIPER_COMMIT="639388b6317fc4731e91d53da42aea68fd4166ff"
PIPER_INSTALL_TO="/usr/local"
ONNXRUNTIME_VERSION="1.22.0"

case "$(uname -m)" in
  'x86_64')
    ONNXRUNTIME_ARCH="x64"
    ONNXRUNTIME_SHA256="8344d55f93d5bc5021ce342db50f62079daf39aaafb5d311a451846228be49b3"
    ;;
  'aarch64')
    ONNXRUNTIME_ARCH="aarch64"
    ONNXRUNTIME_SHA256="bb76395092d150b52c7092dc6b8f2fe4d80f0f3bf0416d2f269193e347e24702"
    ;;
  *)
    echo "Unsupported architecture for libpiper: $(uname -m)" >&2
    exit 1
    ;;
esac

ONNXRUNTIME_NAME="onnxruntime-linux-${ONNXRUNTIME_ARCH}-${ONNXRUNTIME_VERSION}"
ONNXRUNTIME_URL="https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${ONNXRUNTIME_NAME}.tgz"

rm -rf "${ONNXRUNTIME_NAME}" "${ONNXRUNTIME_NAME}.tgz"
DOWNLOADED=0
for _ in 1 2 3 4 5; do
  if curl -fL -o "${ONNXRUNTIME_NAME}.tgz" "${ONNXRUNTIME_URL}"; then
    DOWNLOADED=1
    break
  fi
  sleep 10
done
[ "${DOWNLOADED}" -eq 1 ]
echo "${ONNXRUNTIME_SHA256}  ${ONNXRUNTIME_NAME}.tgz" | sha256sum -c
tar -xzf "${ONNXRUNTIME_NAME}.tgz"
ONNXRUNTIME_DIR="$(pwd)/${ONNXRUNTIME_NAME}"

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
  -DPIPER_BUILD_EXE=OFF \
  -DCMAKE_INSTALL_PREFIX="${PIPER_INSTALL_TO}" \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DONNXRUNTIME_DIR="${ONNXRUNTIME_DIR}"
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
rm -rf piper-source "${ONNXRUNTIME_NAME}" "${ONNXRUNTIME_NAME}.tgz"

ldconfig
