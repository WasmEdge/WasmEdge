#!/bin/bash
set -e

RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
NC=$'\e[0m' # No Color

VERSION="0.8.1"

ARGS=("$@")

if [ $# -ne 1 ]; then
    echo "${NC}${RED}ARGS: ./install_wasmedge_image.sh INSTALL_PATH"
    echo "Ex  : ./install_wasmedge_image.sh /usr/local${NC}"
    exit 1
fi

make_dirs() {
    for var in "$@"; do
        if [ ! -d "${ARGS[0]}/$var" ]; then
            mkdir -p "${ARGS[0]}/$var"
        fi
    done
}

get_wasmedge_image_deps() {
    echo "Fetching WasmEdge-image-deps-$VERSION"
    wget -q https://github.com/second-state/WasmEdge-image/releases/download/$VERSION/WasmEdge-image-deps-$VERSION-manylinux1_x86_64.tar.gz

    tar -C ${ARGS[0]}/lib -zxvf WasmEdge-image-deps-$VERSION-manylinux1_x86_64.tar.gz
    rm -f WasmEdge-image-deps-$VERSION-manylinux1_x86_64.tar.gz
    ln -sf libjpeg.so.8.3.0 ${ARGS[0]}/lib/libjpeg.so
    ln -sf libjpeg.so.8.3.0 ${ARGS[0]}/lib/libjpeg.so.8
    ln -sf libpng16.so.16.37.0 ${ARGS[0]}/lib/libpng.so
    ln -sf libpng16.so.16.37.0 ${ARGS[0]}/lib/libpng16.so
    ln -sf libpng16.so.16.37.0 ${ARGS[0]}/lib/libpng16.so.16
    ldconfig
}

install_wasmedge_image() {
    echo "Fetching WasmEdge-image-$VERSION"
    wget -q https://github.com/second-state/WasmEdge-image/releases/download/$VERSION/WasmEdge-image-$VERSION-manylinux2014_x86_64.tar.gz
    tar -C ${ARGS[0]} -xzf WasmEdge-image-$VERSION-manylinux2014_x86_64.tar.gz
    rm -f WasmEdge-image-$VERSION-manylinux2014_x86_64.tar.gz
    ldconfig
}

make_dirs "lib"
get_wasmedge_image_deps
install_wasmedge_image
echo "${GREEN}Installed in ${ARGS[0]} ${NC}"
