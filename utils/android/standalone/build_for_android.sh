#!/bin/bash

if [ ! $ANDROID_NDK_HOME ]; then
    echo "env var ANDROID_NDK_HOME not set"
    exit 1
else
    echo "use ndk: $ANDROID_NDK_HOME"
fi

WASMEDGE_ROOT_PATH=$(dirname $(dirname $(dirname $(dirname $0))))

cd ${WASMEDGE_ROOT_PATH}
mkdir build
cd build

echo $(pwd)

if ! cmake .. -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_AOT_RUNTIME=OFF -DCMAKE_SYSTEM_NAME=Android -DCMAKE_SYSTEM_VERSION=23 -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a -DCMAKE_ANDROID_NDK=$ANDROID_NDK_HOME -DCMAKE_ANDROID_STL_TYPE=c++_static; then
    echo === CMakeOutput.log ===
    cat build/CMakeFiles/CMakeOutput.log
    echo === CMakeError.log ===
    cat build/CMakeFiles/CMakeError.log
    exit 1
fi

make -j

echo "build finished"
