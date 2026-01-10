#!/bin/bash

if [ ! $ANDROID_NDK_HOME ]; then
    echo "Environment variable ANDROID_NDK_HOME is not set."
    exit 1
else
    echo "Use Android NDK path: $ANDROID_NDK_HOME"
fi

WASMEDGE_ROOT_PATH=$(dirname $(dirname $(dirname $(dirname $0))))

cd ${WASMEDGE_ROOT_PATH}

if ! cmake -Bbuild -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_USE_LLVM=OFF -DCMAKE_SYSTEM_NAME=Android -DCMAKE_SYSTEM_VERSION=23 -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a -DCMAKE_ANDROID_NDK=$ANDROID_NDK_HOME -DCMAKE_ANDROID_STL_TYPE=c++_static; then
    echo === CMakeOutput.log ===
    cat build/CMakeFiles/CMakeOutput.log
    echo === CMakeError.log ===
    cat build/CMakeFiles/CMakeError.log
    exit 1
fi

cmake --build build

echo "Build finished."
