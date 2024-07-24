#!/usr/bin/env bash

set -e

: ${ONNXRUNTIME_VERSION:=1.14.1}

ONNXRUNTIME_NAME="onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}"
ONNXRUNTIME_TGZ="${ONNXRUNTIME_NAME}.tgz"

curl -LO "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${ONNXRUNTIME_TGZ}"
tar zxf "${ONNXRUNTIME_TGZ}"
mv "${ONNXRUNTIME_NAME}/include/"* /usr/local/include/
mv "${ONNXRUNTIME_NAME}/lib/"* /usr/local/lib/
rm -rf "${ONNXRUNTIME_TGZ}" "${ONNXRUNTIME_NAME}"
