#!/bin/bash
set -e

RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
YELLOW=$'\e[0;33m'
NC=$'\e[0m' # No Color
PERM_ROOT=1

if [[ $EUID -ne 0 ]]; then
    echo "${YELLOW}No root permissions.${NC}"
    PERM_ROOT=0
fi

_ldconfig() {
    if [ $PERM_ROOT == 1 ]; then
        ldconfig $IPATH/lib
    fi
}

_downloader() {
    local url=$1
    if ! command -v wget &>/dev/null; then
        if command -v curl &>/dev/null; then
            curl -L -OC - $url --progress-bar
        else
            echo "${RED}Please install wget or curl${NC}"
            exit 1
        fi
    else
        wget -q -c --show-progress $url
    fi
}

if ! command -v git &>/dev/null; then
    echo "${RED}Please install git${NC}"
    exit 1
fi

if command -v sudo &>/dev/null; then
    if [ $PERM_ROOT == 1 ]; then
        HOME=$(getent passwd $SUDO_USER | cut -d: -f6)
    fi
else
    echo "${YELLOW}sudo could not be found${NC}"
fi

RELEASE_PKG="manylinux2014_x86_64.tar.gz"
ARCH=$(uname -m)
OS=$(uname)
IM_EXT_COMPAT=1
TF_EXT_COMPAT=1

case $OS in
'Linux')
    if [ "$ARCH" = "aarch64" ]; then
        RELEASE_PKG="manylinux2014_$ARCH.tar.gz"
        IM_EXT_COMPAT=0
        TF_EXT_COMPAT=0
    fi
    ;;
*)
    echo "${RED}Detected $OS-$ARCH${NC} - currently unsupported${NC}"
    exit 1
    ;;
esac

echo "Detected $OS-$ARCH"

get_latest_release() {
    local res
    if ! command -v wget &>/dev/null; then
        res="$(curl --silent "https://api.github.com/repos/$1/releases/latest" |
            grep '"tag_name":' |
            sed -E 's/.*"([^"]+)".*/\1/')"
        echo "$res"
    else
        res="$(wget -q -O - https://api.github.com/repos/$1/releases/latest |
            grep '"tag_name":' |
            sed -E 's/.*"([^"]+)".*/\1/')"
        echo "$res"
    fi
}

VERSION=$(get_latest_release wasmedge/wasmedge)
VERSION_IM=$(get_latest_release second-state/WasmEdge-image)
VERSION_IM_DEPS=$(get_latest_release second-state/WasmEdge-image)
VERSION_TF=$(get_latest_release second-state/WasmEdge-tensorflow)
VERSION_TF_DEPS=$(get_latest_release second-state/WasmEdge-tensorflow-deps)
VERSION_TF_TOOLS=$(get_latest_release second-state/WasmEdge-tensorflow-tools)

IPATH="$HOME/.wasmedge"
EXT="none"
VERBOSE=0

ENV="#!/bin/sh
# wasmedge shell setup
# affix colons on either side of \$PATH to simplify matching
case ":\${PATH}:" in
    *:"$IPATH/bin":*)
        ;;
    *)
        # Prepending path in case a system-installed wasmedge needs to be overridden
        export PATH="$IPATH/bin:\$PATH"
        ;;
esac
case ":\${LD_LIBRARY_PATH}:" in
    *:"$IPATH/lib":*)
        ;;
    *)
        # Prepending path in case a system-installed wasmedge libs needs to be overridden
        export LD_LIBRARY_PATH="$IPATH/lib:\$LD_LIBRARY_PATH"
        ;;
esac"

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
    if [ ! $default == 1 ]; then
        rm -rf $IPATH
    fi
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
            mv -f $IPATH/$dir/lib64/* $IPATH/$var
        else
            mv -f $IPATH/$dir/$var/* $IPATH/$var
        fi
    done
}

get_wasmedge_release() {
    echo "Fetching WasmEdge-$1"
    _downloader https://github.com/WasmEdge/WasmEdge/releases/download/$1/WasmEdge-$1-$RELEASE_PKG
    tar -C $IPATH -xzf WasmEdge-$1-$RELEASE_PKG
}

wasmedge_post_install() {
    rm -f WasmEdge-$1-$RELEASE_PKG
    rm -rf $IPATH/WasmEdge-$1-Linux
    _ldconfig
}

wasmedge_checks() {
    # Check only MAJOR.MINOR.PATCH
    if [ $PERM_ROOT == 1 ]; then
        local version=$1
        shift
        for var in "$@"; do
            local V=$($IPATH/bin/$var --version | sed 's/^.*[^0-9]\([0-9]*\.[0-9]*\.[0-9]*\).*$/\1/')
            local V_=$(echo $version | sed 's/\([0-9]*\.[0-9]*\.[0-9]*\).*$/\1/')
            if [ $V = $V_ ]; then
                echo "${GREEN}Installation of $var-$version successfull${NC}"
            else
                echo "${YELLOW}version $V_ does not match $V for $var-$version${NC}"
                exit 1
            fi
        done
    fi
}

get_wasmedge_image_deps() {
    echo "Fetching WasmEdge-image-deps-$VERSION_IM"
    _downloader https://github.com/second-state/WasmEdge-image/releases/download/$VERSION_IM_DEPS/WasmEdge-image-deps-$VERSION_IM_DEPS-manylinux1_x86_64.tar.gz

    tar -C $IPATH/lib -zxvf WasmEdge-image-deps-$VERSION_IM_DEPS-manylinux1_x86_64.tar.gz
    rm -f WasmEdge-image-deps-$VERSION_IM-manylinux1_x86_64.tar.gz
    ln -sf libjpeg.so.8.3.0 $IPATH/lib/libjpeg.so
    ln -sf libjpeg.so.8.3.0 $IPATH/lib/libjpeg.so.8
    ln -sf libpng16.so.16.37.0 $IPATH/lib/libpng.so
    ln -sf libpng16.so.16.37.0 $IPATH/lib/libpng16.so
    ln -sf libpng16.so.16.37.0 $IPATH/lib/libpng16.so.16
    _ldconfig
}

install_wasmedge_image() {
    echo "Fetching WasmEdge-image-$VERSION_IM"
    _downloader https://github.com/second-state/WasmEdge-image/releases/download/$VERSION_IM/WasmEdge-image-$VERSION_IM-$RELEASE_PKG
    tar -C $IPATH -xzf WasmEdge-image-$VERSION_IM-$RELEASE_PKG
    rm -f WasmEdge-image-$VERSION_IM-$RELEASE_PKG
    _ldconfig
}

get_wasmedge_tensorflow_deps() {
    echo "Fetching WasmEdge-tensorflow-deps-TF-$VERSION_TF_DEPS"
    _downloader https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/$VERSION_TF_DEPS/WasmEdge-tensorflow-deps-TF-$VERSION_TF_DEPS-$RELEASE_PKG

    echo "Fetching WasmEdge-tensorflow-deps-TFLite-$VERSION_TF_DEPS"
    _downloader https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/$VERSION_TF_DEPS/WasmEdge-tensorflow-deps-TFLite-$VERSION_TF_DEPS-$RELEASE_PKG

    tar -C $IPATH/lib -zxvf WasmEdge-tensorflow-deps-TF-$VERSION_TF_DEPS-$RELEASE_PKG
    tar -C $IPATH/lib -zxvf WasmEdge-tensorflow-deps-TFLite-$VERSION_TF_DEPS-$RELEASE_PKG
    rm -f WasmEdge-tensorflow-deps-TF-$VERSION_TF_DEPS-$RELEASE_PKG
    rm -f WasmEdge-tensorflow-deps-TFLite-$VERSION_TF_DEPS-$RELEASE_PKG
    ln -sf libtensorflow.so.2.4.0 $IPATH/lib/libtensorflow.so.2
    ln -sf libtensorflow.so.2 $IPATH/lib/libtensorflow.so
    ln -sf libtensorflow_framework.so.2.4.0 $IPATH/lib/libtensorflow_framework.so.2
    ln -sf libtensorflow_framework.so.2 $IPATH/lib/libtensorflow_framework.so
    _ldconfig
}

install_wasmedge_tensorflow() {
    echo "Fetching WasmEdge-tensorflow-$VERSION_TF"
    _downloader https://github.com/second-state/WasmEdge-tensorflow/releases/download/$VERSION_TF/WasmEdge-tensorflow-$VERSION_TF-$RELEASE_PKG

    echo "Fetching WasmEdge-tensorflowlite-$VERSION_TF"
    _downloader https://github.com/second-state/WasmEdge-tensorflow/releases/download/$VERSION_TF/WasmEdge-tensorflowlite-$VERSION_TF-$RELEASE_PKG

    echo "Fetching WasmEdge-tensorflow-tools-$VERSION_TF_TOOLS"
    _downloader https://github.com/second-state/WasmEdge-tensorflow-tools/releases/download/$VERSION_TF_TOOLS/WasmEdge-tensorflow-tools-$VERSION_TF_TOOLS-$RELEASE_PKG

    tar -C $IPATH -xzf WasmEdge-tensorflow-$VERSION_TF-$RELEASE_PKG
    rm -f WasmEdge-tensorflow-$VERSION_TF-$RELEASE_PKG

    tar -C $IPATH -xzf WasmEdge-tensorflowlite-$VERSION_TF-$RELEASE_PKG
    rm -f WasmEdge-tensorflowlite-$VERSION_TF-$RELEASE_PKG

    tar -C $IPATH/bin -xzf WasmEdge-tensorflow-tools-$VERSION_TF_TOOLS-$RELEASE_PKG
    rm -f $IPATH/bin/download_dependencies_all.sh \
        $IPATH/bin/download_dependencies_tf.sh \
        $IPATH/bin/download_dependencies_tflite.sh
    rm -f WasmEdge-tensorflow-tools-$VERSION_TF_TOOLS-$RELEASE_PKG

    _ldconfig
}

install_image_extensions() {
    if [ $IM_EXT_COMPAT == 1 ]; then
        get_wasmedge_image_deps
        install_wasmedge_image
    else
        echo "${YELLOW}Image Extensions not supported${NC}"
    fi
}

install_tf_extensions() {
    if [ $TF_EXT_COMPAT == 1 ]; then
        get_wasmedge_tensorflow_deps
        install_wasmedge_tensorflow
        wasmedge_checks $VERSION_TF_TOOLS wasmedge-tensorflow \
            wasmedgec-tensorflow \
            wasmedge-tensorflow-lite
    else
        echo "${YELLOW}Tensorflow extensions not supported${NC}"
    fi
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
        echo "${YELLOW}No path provided"
        echo "Installing in $IPATH${NC}"
        mkdir -p $IPATH
        echo "$ENV" >$IPATH/env
        local _source=". \"\$HOME/.wasmedge/env\""
        local _grep=$(cat $HOME/.profile 2>/dev/null | grep "wasmedge")
        if [ -f $HOME/.profile ]; then
            if [ "$_grep" = "" ]; then
                echo $_source >>$HOME/.profile
            fi
        elif [ -f $HOME/.bashrc ]; then
            local _grep=$(cat $HOME/.bashrc | grep "wasmedge")
            if [ "$_grep" = "" ]; then
                echo $_source >>$HOME/.bashrc
            fi
        elif [ -f $HOME/.bash_profile ]; then
            local _grep=$(cat $HOME/.bash_profile | grep "wasmedge")
            if [ "$_grep" = "" ]; then
                echo $_source >>$HOME/.bash_profile
            fi
        else
            echo $_source >>$HOME/.bashrc
        fi
    fi

    if [ ! $VERBOSE == 0 ]; then
        echo "Verbose Mode"
        set -xv
    fi

    if [ -d $IPATH ]; then
        echo "WasmEdge Installation at $IPATH"
        make_dirs "include" "lib" "bin"

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
        install_image_extensions
    elif [ "$EXT" = "tf" ]; then
        echo "Tensorflow Extensions"
        install_tf_extensions
    elif [ "$EXT" = "all" ]; then
        echo "Image & Tensorflow extensions"
        install_image_extensions
        install_tf_extensions
    elif [ "$EXT" = "none" ]; then
        echo "No extensions to be installed"
    else
        echo "Invalid extension"
    fi

    trap - EXIT
    end_message
}

end_message() {
    if [ ! $default == 1 ]; then
        echo ""
        echo "${GREEN}source $IPATH/env${NC} to use wasmedge binaries"
    else
        case ":${PATH}:" in
        *:"${IPATH%"/"}/bin":*)
            echo "${GREEN}WasmEdge binaries accessible${NC}"
            ;;
        *)
            echo "${YELLOW}Add $IPATH/bin to your PATH using following command"
            echo "export PATH=$IPATH/bin:\$PATH${NC}"
            ;;
        esac
    fi
}

main "$@"
