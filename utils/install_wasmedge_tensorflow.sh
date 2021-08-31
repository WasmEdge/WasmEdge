#!/bin/bash
set -e

RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
NC=$'\e[0m' # No Color

VERSION="0.8.1"
VERSION_TF="0.8.0"

ARGS=("$@")

if [ $# -ne 1 ]; then
    echo "${NC}${RED}ARGS: ./install_wasmedge_tensorflow.sh INSTALL_PATH"
    echo "Ex  : ./install_wasmedge_tensorflow.sh /usr/local${NC}"
    exit 1
fi

make_dirs() {
    for var in "$@"; do
        if [ ! -d "${ARGS[0]}/$var" ]; then
            mkdir -p "${ARGS[0]}/$var"
        fi
    done
}

get_wasmedge_tensorflow_deps() {
    echo "Fetching WasmEdge-tensorflow-deps-TF-$VERSION_TF"
    wget -q https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/$VERSION_TF/WasmEdge-tensorflow-deps-TF-$VERSION_TF-manylinux2014_x86_64.tar.gz

    echo "Fetching WasmEdge-tensorflow-deps-TFLite-$VERSION_TF"
    wget -q https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/$VERSION_TF/WasmEdge-tensorflow-deps-TFLite-$VERSION_TF-manylinux2014_x86_64.tar.gz

    tar -C ${ARGS[0]}/lib -zxvf WasmEdge-tensorflow-deps-TF-$VERSION_TF-manylinux2014_x86_64.tar.gz
    tar -C ${ARGS[0]}/lib -zxvf WasmEdge-tensorflow-deps-TFLite-$VERSION_TF-manylinux2014_x86_64.tar.gz
    rm -f WasmEdge-tensorflow-deps-TF-$VERSION_TF-manylinux2014_x86_64.tar.gz
    rm -f WasmEdge-tensorflow-deps-TFLite-$VERSION_TF-manylinux2014_x86_64.tar.gz
    ln -sf libtensorflow.so.2.4.0 ${ARGS[0]}/lib/libtensorflow.so.2
    ln -sf libtensorflow.so.2 ${ARGS[0]}/lib/libtensorflow.so
    ln -sf libtensorflow_framework.so.2.4.0 ${ARGS[0]}/lib/libtensorflow_framework.so.2
    ln -sf libtensorflow_framework.so.2 ${ARGS[0]}/lib/libtensorflow_framework.so
    ldconfig
}

install_wasmedge_tensorflow() {
    echo "Fetching WasmEdge-tensorflow-$VERSION"
    wget -q https://github.com/second-state/WasmEdge-tensorflow/releases/download/$VERSION/WasmEdge-tensorflow-$VERSION-manylinux2014_x86_64.tar.gz

    echo "Fetching WasmEdge-tensorflowlite-$VERSION"
    wget -q https://github.com/second-state/WasmEdge-tensorflow/releases/download/$VERSION/WasmEdge-tensorflowlite-$VERSION-manylinux2014_x86_64.tar.gz

    tar -C ${ARGS[0]} -xzf WasmEdge-tensorflow-$VERSION-manylinux2014_x86_64.tar.gz
    tar -C ${ARGS[0]} -xzf WasmEdge-tensorflowlite-$VERSION-manylinux2014_x86_64.tar.gz
    rm -f WasmEdge-tensorflow-$VERSION-manylinux2014_x86_64.tar.gz
    rm -f WasmEdge-tensorflowlite-$VERSION-manylinux2014_x86_64.tar.gz
    ldconfig
}

make_dirs "lib"
get_wasmedge_tensorflow_deps
install_wasmedge_tensorflow
echo "${GREEN}Installed in ${ARGS[0]} ${NC}"
