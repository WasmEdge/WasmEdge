#!/bin/bash
set -e

RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
NC=$'\e[0m' # No Color

VERSION="0.8.1"

ARGS=("$@")

if [ $# -ne 1 ]; then
    echo "${NC}${RED}ARGS: ./install_wasmedge.sh INSTALL_PATH"
    echo "Ex  : ./install_wasmedge.sh /usr/local${NC}"
    exit 1
fi

make_dirs() {
    for var in "$@"; do
        if [ ! -d "${ARGS[0]}/$var" ]; then
            mkdir -p "${ARGS[0]}/$var"
        fi
    done
}

get_wasmedge_release() {
    echo "Fetching WasmEdge-$1"
    wget -q https://github.com/WasmEdge/WasmEdge/releases/download/$1/WasmEdge-$1-manylinux2014_x86_64.tar.gz
    tar -C ${ARGS[0]} -xzf WasmEdge-$1-manylinux2014_x86_64.tar.gz
}

deps_install() {
    set +e
    apt update
    for var in "$@"; do
        PKG_OK=$(dpkg-query -W --showformat='${Status}\n' $var | grep "install ok installed")
        echo Checking for $var: $PKG_OK
        if [ "" = "$PKG_OK" ]; then
            echo "No $var. Setting up $var."
            apt -y install $var
        fi
    done
    set -e
}

install() {
    for var in "$@"; do
        echo "${GREEN}Installing in ${ARGS[0]}/$var ${NC}"
        if [ $var = "lib" ]; then
            mv ${ARGS[0]}/WasmEdge-$VERSION-Linux/lib64/* ${ARGS[0]}/$var
        else
            mv ${ARGS[0]}/WasmEdge-$VERSION-Linux/$var/* ${ARGS[0]}/$var
        fi
    done
}

post_install() {
    rm -f WasmEdge-$1-manylinux2014_x86_64.tar.gz
    rm -rf ${ARGS[0]}/WasmEdge-$1-Linux
    ldconfig
}

checks() {
    for var in "$@"; do
        V=$($var --version)
        if [ "$V" = "$var version $VERSION" ]; then
            echo "${GREEN}Installed $var successfully in ${ARGS[0]}/bin ${NC}"
        else
            echo "${RED}ERROR INSTALLING $var $VERSION"
            echo "Output $V  ${NC}"
            exit 1
        fi
    done
}

make_dirs "include" "lib" "bin"
deps_install software-properties-common \
    wget \
    cmake \
    ninja-build \
    curl \
    git \
    dpkg-dev \
    libboost-all-dev \
    llvm-12-dev \
    liblld-12-dev \
    gcc \
    g++

get_wasmedge_release $VERSION
install "include" "lib" "bin"
post_install $VERSION
checks "wasmedge" "wasmedgec"
