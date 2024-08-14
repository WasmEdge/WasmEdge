#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

set -e

case "$(uname -m)" in
    'x86_64') ARCH='x64' ;;
    'aarch64') ARCH='aarch64' ;;
    *)
        echo 'Cannot determine architecture for onnxruntime' >&2
        exit 1
        ;;
esac

: ${ONNXRUNTIME_VERSION:=1.14.1}

ONNXRUNTIME_NAME="onnxruntime-linux-${ARCH}-${ONNXRUNTIME_VERSION}"
ONNXRUNTIME_TGZ="${ONNXRUNTIME_NAME}.tgz"

curl -LO "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${ONNXRUNTIME_TGZ}"
tar zxf "${ONNXRUNTIME_TGZ}"
mv "${ONNXRUNTIME_NAME}/include/"* /usr/local/include/
mv "${ONNXRUNTIME_NAME}/lib/"* /usr/local/lib/
rm -rf "${ONNXRUNTIME_TGZ}" "${ONNXRUNTIME_NAME}"

ldconfig
