#!/bin/bash
set -e

echo "::group::Build and install libpiper"
rm -rf piper-source

ONNXRUNTIME_VERSION="1.22.0"

case "$(uname -m)" in
  'x86_64')
    ONNXRUNTIME_ARCH="x86_64"
    ONNXRUNTIME_SHA256="e4ec94a7696de74fb1b12846569aa94e499958af6ffa186022cfde16c9d617f0"
    ;;
  'arm64')
    ONNXRUNTIME_ARCH="arm64"
    ONNXRUNTIME_SHA256="cab6dcbd77e7ec775390e7b73a8939d45fec3379b017c7cb74f5b204c1a1cc07"
    ;;
  *)
    echo "Unsupported architecture for libpiper: $(uname -m)" >&2
    exit 1
    ;;
esac

ONNXRUNTIME_NAME="onnxruntime-osx-${ONNXRUNTIME_ARCH}-${ONNXRUNTIME_VERSION}"
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
echo "${ONNXRUNTIME_SHA256}  ${ONNXRUNTIME_NAME}.tgz" | shasum -a 256 -c
tar -xzf "${ONNXRUNTIME_NAME}.tgz"
ONNXRUNTIME_DIR="$PWD/${ONNXRUNTIME_NAME}"

git clone https://github.com/OHF-Voice/piper1-gpl piper-source
cd piper-source/libpiper
git checkout 639388b6317fc4731e91d53da42aea68fd4166ff
cmake -Bbuild-deps \
  -DCMAKE_BUILD_TYPE=Release \
  -DPIPER_BUILD_EXE=OFF \
  -DCMAKE_INSTALL_PREFIX="$PWD/install" \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DONNXRUNTIME_DIR="${ONNXRUNTIME_DIR}"

cmake --build build-deps
cmake --install build-deps

cd ../..
rm -rf "${ONNXRUNTIME_NAME}.tgz"
echo "::endgroup::"
