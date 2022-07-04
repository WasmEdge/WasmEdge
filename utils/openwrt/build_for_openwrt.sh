#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

OPENWRT_DIR_PATH=$1
WASMEDGE_FATHER_PATH=$(dirname $(dirname $(dirname $(dirname $(pwd)))))

cd ${OPENWRT_DIR_PATH}/package/utils
mkdir WasmEdge && cd WasmEdge
cp -r ${WASMEDGE_FATHER_PATH}/WasmEdge ${OPENWRT_DIR_PATH}/package/utils/WasmEdge/src
cp ${OPENWRT_DIR_PATH}/package/utils/WasmEdge/src/utils/openwrt/configuration/Makefile ${OPENWRT_DIR_PATH}/package/utils/WasmEdge/
cd ${OPENWRT_DIR_PATH}

make menuconfig

make package/utils/WasmEdge/compile V=s
