#!/bin/bash

export PATH=$PATH:/home/openharmony/prebuilts/clang/ohos/linux-x86_64/llvm/bin:/home/openharmony/prebuilts/cmake/linux-x86/bin/
export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/home/openharmony/third_party/boost/

cp ./* $(dirname $(dirname $(pwd)))

cd $(dirname $(dirname $(pwd)))

mkdir build
cd build
if ! cmake .. -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_AOT_RUNTIME=OFF -DWASMEDGE_BUILD_ON_OHOS=ON -DOHOS_SYSROOT_PATH="/home/openharmony/out/ohos-arm-release/obj/third_party/musl/" -DBoost_NO_SYSTEM_PATHS=TRUE -DBOOST_INCLUDEDIR="/home/openharmony/third_party/boost/"; then
    echo === CMakeOutput.log ===
    cat build/CMakeFiles/CMakeOutput.log
    echo === CMakeError.log ===
    cat build/CMakeFiles/CMakeError.log
    exit 1
fi

make -j

#build ohos
cd /home/openharmony
./build.sh --product-name Hi3516DV300 --ccache
