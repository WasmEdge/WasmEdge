#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

OHOS_DIR_PATH=$1
WASMEDGE_ROOT_PATH=$(dirname $(dirname $(pwd)))

export PATH=$PATH:${OHOS_DIR_PATH}/prebuilts/clang/ohos/linux-x86_64/llvm/bin:${OHOS_DIR_PATH}/prebuilts/cmake/linux-x86/bin/
export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:${OHOS_DIR_PATH}/third_party/boost/

cp ./configuration/* ${WASMEDGE_ROOT_PATH}

cd ${WASMEDGE_ROOT_PATH}

mkdir build
cd build
if ! cmake .. -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_AOT_RUNTIME=OFF -DWASMEDGE_BUILD_ON_OHOS=ON -DOHOS_DIR_PATH=${OHOS_DIR_PATH} -DOHOS_SYSROOT_PATH="${OHOS_DIR_PATH}/out/ohos-arm-release/obj/third_party/musl/" -DBoost_NO_SYSTEM_PATHS=TRUE -DBOOST_INCLUDEDIR="${OHOS_DIR_PATH}/third_party/boost/"; then
    echo === CMakeOutput.log ===
    cat build/CMakeFiles/CMakeOutput.log
    echo === CMakeError.log ===
    cat build/CMakeFiles/CMakeError.log
    exit 1
fi

make -j

#build ohos
cd ${OHOS_DIR_PATH}
./build.sh --product-name Hi3516DV300 --ccache
