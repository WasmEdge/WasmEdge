#!/bin/bash
set -e

RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
YELLOW=$'\e[0;33m'
NC=$'\e[0m' # No Color
DEB_F_SET=0

if ! command -v git &>/dev/null; then
    echo "${YELLOW}git could not be found${NC}"
    if [ $DEBIAN_FRONTEND="" ]; then
        export DEBIAN_FRONTEND="noninteractive"
        DEB_F_SET=1
    fi
    apt update
    apt install -y git
fi

VERSION=$(git -c 'versionsort.suffix=-' \
    ls-remote --exit-code --refs --sort='version:refname' --tags https://github.com/WasmEdge/WasmEdge.git '*.*.*' |
    tail --lines=1 |
    cut --delimiter='/' --fields=3)
VERSION_IM=$(git -c 'versionsort.suffix=-' \
    ls-remote --exit-code --refs --sort='version:refname' --tags https://github.com/second-state/WasmEdge-image.git '*.*.*' |
    tail --lines=1 |
    cut --delimiter='/' --fields=3)
VERSION_IM_DEPS=$(git -c 'versionsort.suffix=-' \
    ls-remote --exit-code --refs --sort='version:refname' --tags https://github.com/second-state/WasmEdge-image.git '*.*.*' |
    tail --lines=1 |
    cut --delimiter='/' --fields=3)
VERSION_TF=$(git -c 'versionsort.suffix=-' \
    ls-remote --exit-code --refs --sort='version:refname' --tags https://github.com/second-state/WasmEdge-tensorflow.git '*.*.*' |
    tail --lines=1 |
    cut --delimiter='/' --fields=3)
VERSION_TF_DEPS=$(git -c 'versionsort.suffix=-' \
    ls-remote --exit-code --refs --sort='version:refname' --tags https://github.com/second-state/WasmEdge-tensorflow-deps.git '*.*.*' |
    tail --lines=1 |
    cut --delimiter='/' --fields=3)
VERSION_TF_TOOLS=$(git -c 'versionsort.suffix=-' \
    ls-remote --exit-code --refs --sort='version:refname' --tags https://github.com/second-state/WasmEdge-tensorflow-tools.git '*.*.*' |
    tail --lines=1 |
    cut --delimiter='/' --fields=3)

IPATH="/usr/local"
EXT="none"
VERBOSE=0

usage() {
    cat <<EOF
    Usage: $0 -p </path/to/install> [-V]
    WasmEdge installation, uninstallation and extensions install.
    Mandatory arguments to long options are mandatory for short options too.

    -h, -help,          --help                      Display help

    -p, -path,          --path=[/usr/local]         Prefix / Path to install

    -v, -version,       --version=VERSION           Set and Download specific 
                                                    version of WasmEdge
                        
                        --tf-version=VERSION_TF
                        --tf-deps-version==VERSION_TF_DEPS
                        --tf-tools-version=VERSION_TF_TOOLS
                        --image-version=VERSION_IM
                        --image-deps-version=VERSION_IM_DEPS

    -e, -extension,     --extension=[tf|image|all|none]  
                                                    Enable extension support 
                                                    i.e Tensorflow (tf) 
                                                        or Image (image)

    -V, -verbose,       --verbose                   Run script in verbose mode.
                                                    Will print out each step 
                                                    of execution.

    Example:
    ./$0 -p $IPATH -e all -v $VERSION --verbose

    About:

    - wasmedgec is the AOT compiler that compiles WebAssembly bytecode programs 
      (wasm programs) into native code (so program) on your deployment machine.
    
    - wasmedge is the runtime that executes the wasm program or the AOT compiled
      so program.

    - wasmedgec-tensorflow is the AOT compiler that compiles WebAssembly 
      bytecode programs (wasm programs) into native code (so program) on your 
      deployment machine. It is aware of WamsEdge's Tensorflow extension API.
    
    - wasmedge-tensorflow-lite is the runtime that executes the wasm program or 
      the AOT compiled so program with the Tensorflow Lite library.

EOF
}

on_exit() {
    cat <<EOF
${RED}
    Please see --help
    If issue perists make a trace using -V and submit it to
https://github.com/WasmEdge/WasmEdge/issues/new?assignees=&labels=&template=bug_report.md
${NC}
EOF

}

make_dirs() {
    for var in "$@"; do
        if [ ! -d "$IPATH/$var" ]; then
            mkdir -p "$IPATH/$var"
        fi
    done
}

install() {
    local dir=$1
    shift
    for var in "$@"; do
        echo "${GREEN}Installing $dir in $IPATH/$var ${NC}"
        if [ $var = "lib" ]; then
            mv $IPATH/$dir/lib64/* $IPATH/$var
        else
            mv $IPATH/$dir/$var/* $IPATH/$var
        fi
    done
}

wasmedge_deps_install() {
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
    if [ "$DEBIAN_FRONTEND" = "noninteractive" ] && [ $DEB_F_SET -eq 1 ]; then
        unset DEBIAN_FRONTEND
    fi
}

get_wasmedge_release() {
    echo "Fetching WasmEdge-$1"
    wget -q -c --show-progress https://github.com/WasmEdge/WasmEdge/releases/download/$1/WasmEdge-$1-manylinux2014_x86_64.tar.gz
    tar -C $IPATH -xzf WasmEdge-$1-manylinux2014_x86_64.tar.gz
}

wasmedge_post_install() {
    rm -f WasmEdge-$1-manylinux2014_x86_64.tar.gz
    rm -rf $IPATH/WasmEdge-$1-Linux
    ldconfig
}

wasmedge_checks() {
    # Check only MAJOR.MINOR.PATCH
    local version=$1
    shift
    for var in "$@"; do
        local V=$($var --version | sed 's/^.*[^0-9]\([0-9]*\.[0-9]*\.[0-9]*\).*$/\1/')
        local V_=$(echo $version | sed 's/\([0-9]*\.[0-9]*\.[0-9]*\).*$/\1/')
        if [ $V = $V_ ]; then
            echo "${GREEN}Installation of $var-$version successfull${NC}"
        else
            echo "${YELLOW}version $V_ does not match $V for $var-$version${NC}"
            exit 1
        fi
    done
}

get_wasmedge_image_deps() {
    echo "Fetching WasmEdge-image-deps-$VERSION_IM"
    wget -q -c --show-progress https://github.com/second-state/WasmEdge-image/releases/download/$VERSION_IM_DEPS/WasmEdge-image-deps-$VERSION_IM_DEPS-manylinux1_x86_64.tar.gz

    tar -C $IPATH/lib -zxvf WasmEdge-image-deps-$VERSION_IM_DEPS-manylinux1_x86_64.tar.gz
    rm -f WasmEdge-image-deps-$VERSION_IM-manylinux1_x86_64.tar.gz
    ln -sf libjpeg.so.8.3.0 $IPATH/lib/libjpeg.so
    ln -sf libjpeg.so.8.3.0 $IPATH/lib/libjpeg.so.8
    ln -sf libpng16.so.16.37.0 $IPATH/lib/libpng.so
    ln -sf libpng16.so.16.37.0 $IPATH/lib/libpng16.so
    ln -sf libpng16.so.16.37.0 $IPATH/lib/libpng16.so.16
    ldconfig
}

install_wasmedge_image() {
    echo "Fetching WasmEdge-image-$VERSION_IM"
    wget -q -c --show-progress https://github.com/second-state/WasmEdge-image/releases/download/$VERSION_IM/WasmEdge-image-$VERSION_IM-manylinux2014_x86_64.tar.gz
    tar -C $IPATH -xzf WasmEdge-image-$VERSION_IM-manylinux2014_x86_64.tar.gz
    rm -f WasmEdge-image-$VERSION_IM-manylinux2014_x86_64.tar.gz
    ldconfig
}

get_wasmedge_tensorflow_deps() {
    echo "Fetching WasmEdge-tensorflow-deps-TF-$VERSION_TF_DEPS"
    wget -q -c --show-progress https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/$VERSION_TF_DEPS/WasmEdge-tensorflow-deps-TF-$VERSION_TF_DEPS-manylinux2014_x86_64.tar.gz

    echo "Fetching WasmEdge-tensorflow-deps-TFLite-$VERSION_TF_DEPS"
    wget -q -c --show-progress https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/$VERSION_TF_DEPS/WasmEdge-tensorflow-deps-TFLite-$VERSION_TF_DEPS-manylinux2014_x86_64.tar.gz

    tar -C $IPATH/lib -zxvf WasmEdge-tensorflow-deps-TF-$VERSION_TF_DEPS-manylinux2014_x86_64.tar.gz
    tar -C $IPATH/lib -zxvf WasmEdge-tensorflow-deps-TFLite-$VERSION_TF_DEPS-manylinux2014_x86_64.tar.gz
    rm -f WasmEdge-tensorflow-deps-TF-$VERSION_TF_DEPS-manylinux2014_x86_64.tar.gz
    rm -f WasmEdge-tensorflow-deps-TFLite-$VERSION_TF_DEPS-manylinux2014_x86_64.tar.gz
    ln -sf libtensorflow.so.2.4.0 $IPATH/lib/libtensorflow.so.2
    ln -sf libtensorflow.so.2 $IPATH/lib/libtensorflow.so
    ln -sf libtensorflow_framework.so.2.4.0 $IPATH/lib/libtensorflow_framework.so.2
    ln -sf libtensorflow_framework.so.2 $IPATH/lib/libtensorflow_framework.so
    ldconfig
}

install_wasmedge_tensorflow() {
    echo "Fetching WasmEdge-tensorflow-$VERSION_TF"
    wget -q -c --show-progress https://github.com/second-state/WasmEdge-tensorflow/releases/download/$VERSION_TF/WasmEdge-tensorflow-$VERSION_TF-manylinux2014_x86_64.tar.gz

    echo "Fetching WasmEdge-tensorflowlite-$VERSION_TF"
    wget -q -c --show-progress https://github.com/second-state/WasmEdge-tensorflow/releases/download/$VERSION_TF/WasmEdge-tensorflowlite-$VERSION_TF-manylinux2014_x86_64.tar.gz

    echo "Fetching WasmEdge-tensorflow-tools-$VERSION_TF_TOOLS"
    wget -q -c --show-progress https://github.com/second-state/WasmEdge-tensorflow-tools/releases/download/$VERSION_TF_TOOLS/WasmEdge-tensorflow-tools-$VERSION_TF_TOOLS-manylinux2014_x86_64.tar.gz

    tar -C $IPATH -xzf WasmEdge-tensorflow-$VERSION_TF-manylinux2014_x86_64.tar.gz
    rm -f WasmEdge-tensorflow-$VERSION_TF-manylinux2014_x86_64.tar.gz

    tar -C $IPATH -xzf WasmEdge-tensorflowlite-$VERSION_TF-manylinux2014_x86_64.tar.gz
    rm -f WasmEdge-tensorflowlite-$VERSION_TF-manylinux2014_x86_64.tar.gz

    tar -C $IPATH/bin -xzf WasmEdge-tensorflow-tools-$VERSION_TF_TOOLS-manylinux2014_x86_64.tar.gz
    rm -f $IPATH/bin/download_dependencies_all.sh \
        $IPATH/bin/download_dependencies_tf.sh \
        $IPATH/bin/download_dependencies_tflite.sh
    rm -f WasmEdge-tensorflow-tools-$VERSION_TF_TOOLS-manylinux2014_x86_64.tar.gz

    ldconfig
}

main() {

    trap on_exit EXIT

    # getopt is in the util-linux package,
    # it'll probably be fine, but it's of course a good thing to keep in mind.

    options=$(getopt -l \
        "extension:,help,path:,version:,verbose,tf-version:,tf-deps-version:,tf-tools-version:,image-version:,image-deps-version:" \
        -o "e:hp:v:V" -a -- "$@")

    eval set -- "$options"

    local default=0

    while true; do
        case $1 in
        -e | --extension)
            shift
            EXT=$1
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        -v | --version)
            shift
            VERSION=$1
            ;;
        -V | --verbose)
            VERBOSE=1
            ;;
        -p | --path)
            shift
            IPATH=$1
            default=1
            ;;
        --tf-version)
            shift
            VERSION_TF=$1
            ;;
        --tf-deps-version)
            shift
            VERSION_TF_DEPS=$1
            ;;
        --tf-tools-version)
            shift
            VERSION_TF_TOOLS=$1
            ;;
        --image-version)
            shift
            VERSION_IM=$1
            ;;
        --image-deps-version)
            shift
            VERSION_IM_DEPS=$1
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Internal error!"
            exit 1
            ;;
        esac
        shift
    done

    if [ ! $default == 1 ]; then
        while true; do
            echo "No path provided"
            read -p "Do you wish to install this program in $IPATH?" yn
            case $yn in
            [Yy]*)
                break
                ;;
            [Nn]*) exit 1 ;;
            *) echo "Please answer [Y/N | y/n]" ;;
            esac
        done
    fi

    if [ ! $VERBOSE == 0 ]; then
        echo "Verbose Mode"
        set -xv
    fi

    if [ -d $IPATH ]; then
        echo "WasmEdge Installation at $IPATH"
        make_dirs "include" "lib" "bin"
        wasmedge_deps_install software-properties-common \
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
        install WasmEdge-$VERSION-Linux "include" "lib" "bin"
        wasmedge_post_install $VERSION
        wasmedge_checks $VERSION "wasmedge" "wasmedgec"
    else
        echo "Installation path invalid"
        exit 1
    fi

    if [ "$EXT" = "image" ]; then
        echo "Image Extensions"
        get_wasmedge_image_deps
        install_wasmedge_image
    elif [ "$EXT" = "tf" ]; then
        echo "Tensorflow Extensions"
        get_wasmedge_tensorflow_deps
        install_wasmedge_tensorflow
        wasmedge_checks $VERSION_TF_TOOLS wasmedge-tensorflow \
            wasmedgec-tensorflow \
            wasmedge-tensorflow-lite
    elif [ "$EXT" = "all" ]; then
        echo "Image & Tensorflow extensions"
        get_wasmedge_image_deps
        install_wasmedge_image
        get_wasmedge_tensorflow_deps
        install_wasmedge_tensorflow
        wasmedge_checks $VERSION_TF_TOOLS wasmedge-tensorflow \
            wasmedgec-tensorflow \
            wasmedge-tensorflow-lite
    elif [ "$EXT" = "none" ]; then
        echo "No extensions to be installed"
    else
        echo "Invalid extension"
    fi

    trap - EXIT

}

main "$@"
