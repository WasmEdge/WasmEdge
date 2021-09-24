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
        ldconfig "$IPATH"/lib
    fi
}

if command -v sudo &>/dev/null; then
    if [ $PERM_ROOT == 1 ]; then
        __HOME__=$(getent passwd "$SUDO_USER" | cut -d: -f6)
    fi
else
    echo "${YELLOW}sudo could not be found${NC}"
fi

if [ "$__HOME__" = "" ]; then
    __HOME__="$HOME"
fi

IPATH="$__HOME__/.wasmedge"
EXT="none"
VERBOSE=0
ASK=1

usage() {
    cat <<EOF
    Usage: $0 -p </path/to/uninstall> [-V]
    WasmEdge uninstallation and extensions uninstall.
    Mandatory arguments to long options are mandatory for short options too.

    -h, -help,          --help                      Display help

    -p, -path,          --path=[/usr/local]         Prefix / Path to uninstall

    -q, -quick,           --quick                   Uninstall everything
                                                    without asking

    -e, -extension,     --extension=[tf|image|all|none]  
                                                    Enable extension support 
                                                    i.e Tensorflow (tf) 
                                                        or Image (image)

    -V, -verbose,       --verbose                   Run script in verbose mode.
                                                    Will print out each step 
                                                    of execution.

    Example:
    ./$0 -p $IPATH -e all -v --quick

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

detect_bin_path() {
    set +e
    _path=$(which "$1")
    if [ "$_path" = "" ]; then
        if [ ! -d "$IPATH" ]; then
            echo "${RED}Cannot detect installation path${NC}"
            exit 1
        else
            echo "${GREEN}Installation path found at $IPATH${NC}"
        fi
    else
        IPATH=${_path%"/bin/$1"}
        echo "${GREEN}Installation path found at $IPATH${NC}"
    fi
    set -e
}

ask_remove() {
    local libs=("$@")
    if [ $ASK == 1 ]; then
        while true; do
            echo "Do you wish to uninstall the following libs?"
            for var in "${libs[@]}"; do
                echo "  $var"
            done
            read -p "Please answer [Y/N | y/n]" yn
            case $yn in
            [Yy]*)
                for var in "${libs[@]}"; do
                    echo "Removing $var"
                    rm -f "$var"
                done
                ldconfig
                break
                ;;
            [Nn]*)
                echo "Aborted Uninstalling wasmedge_image_deps"
                break
                ;;
            *) echo "Please answer [Y/N | y/n]" ;;
            esac
        done
    else
        for var in "${libs[@]}"; do
            echo "Removing $var"
            rm -f "$var"
        done
        _ldconfig
    fi
}

uninstall_wasmedge() {
    echo "Uninstalling WasmEdge"
    rm -f "$IPATH"/include/wasmedge.h
    rm -f "$IPATH"/lib/libwasmedge_c.so
    rm -f "$IPATH"/bin/wasmedge
    rm -f "$IPATH"/bin/wasmedgec
    _ldconfig
}

wasmedge_checks() {
    for var in "$@"; do
        if [ -f "$IPATH"/bin/"$var" ]; then
            echo "${RED}Uninstallation of $var unsuccessfull${NC}"
            exit 1
        else
            echo "${GREEN}Uninstallation of $var successfull${NC}"
        fi
    done
}

remove_wasmedge_image_deps() {
    echo "
    Uninstall
    WasmEdge-image-deps"
    local libs=("$IPATH"/lib/libjpeg.so
        "$IPATH"/lib/libjpeg.so.8
        "$IPATH"/lib/libjpeg.so.8.3.0
        "$IPATH"/lib/libpng.so
        "$IPATH"/lib/libpng16.so
        "$IPATH"/lib/libpng16.so.16
        "$IPATH"/lib/libpng16.so.16.37.0)

    ask_remove "${libs[@]}"
}

uninstall_wasmedge_image() {
    echo "
    Uninstalling ===>
    WasmEdge-image"
    rm -f "$IPATH"/lib/libwasmedge-image_c.so
    rm -f "$IPATH"/include/wasmedge-image.h
    _ldconfig
}

remove_wasmedge_tensorflow_deps() {
    echo "
    Uninstall
    WasmEdge-tensorflow-deps-TF 
    WasmEdge-tensorflow-deps-TFLite"
    local libs=("$IPATH"/lib/libtensorflow.so.2
        "$IPATH"/lib/libtensorflow.so
        "$IPATH"/lib/libtensorflow_framework.so.2
        "$IPATH"/lib/libtensorflow_framework.so
        "$IPATH"/lib/libtensorflow.so.2.4.0
        "$IPATH"/lib/libtensorflow.so.2
        "$IPATH"/lib/libtensorflow_framework.so.2.4.0
        "$IPATH"/lib/libtensorflowlite_c.so
        "$IPATH"/lib/libtensorflow_framework.so.2)

    ask_remove "${libs[@]}"
}

uninstall_wasmedge_tensorflow() {
    echo "
    Uninstalling ===>
    WasmEdge-tensorflow  
    WasmEdge-tensorflowlite 
    WasmEdge-tensorflow-tools"
    rm -f "$IPATH"/include/wasmedge-tensorflow.h
    rm -f "$IPATH"/include/wasmedge-tensorflowlite.h
    rm -f "$IPATH"/lib/libwasmedge-tensorflow_c.so
    rm -f "$IPATH"/lib/libwasmedge-tensorflowlite_c.so
    rm -f "$IPATH"/bin/wasmedge-tensorflow-lite
    rm -f "$IPATH"/bin/wasmedge-tensorflow
    rm -f "$IPATH"/bin/wasmedgec-tensorflow
    rm -f "$IPATH"/bin/show-tflite-tensor
    _ldconfig
}

main() {

    # getopt is in the util-linux package,
    # it'll probably be fine, but it's of course a good thing to keep in mind.

    options=$(getopt -l "extension:,help,path:,quick,verbose" -o "e:hp:qV" -a -- "$@")

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
        -V | --verbose)
            VERBOSE=1
            ;;
        -p | --path)
            shift
            IPATH=$1
            default=1
            ;;
        -q | --quick)
            ASK=0
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

    detect_bin_path wasmedge

    if [ ! $default == 1 ] && [ ! $ASK == 0 ]; then
        while true; do
            echo "No path provided"
            read -p "Do you wish to uninstall this program from $IPATH?" yn
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

    if [ -d "$IPATH" ]; then
        echo "WasmEdge uninstallation from $IPATH"

        uninstall_wasmedge
        wasmedge_checks "wasmedge" "wasmedgec"
    else
        echo "Uninstallation path invalid"
        exit 1
    fi

    if [ "$EXT" = "image" ]; then
        echo "Image Extensions"
        remove_wasmedge_image_deps
        uninstall_wasmedge_image
    elif [ "$EXT" = "tf" ]; then
        echo "Tensorflow Extensions"
        remove_wasmedge_tensorflow_deps
        uninstall_wasmedge_tensorflow
        wasmedge_checks wasmedge-tensorflow \
            wasmedgec-tensorflow \
            wasmedge-tensorflow-lite
    elif [ "$EXT" = "all" ]; then
        echo "Image & Tensorflow extensions"
        remove_wasmedge_image_deps
        uninstall_wasmedge_image
        remove_wasmedge_tensorflow_deps
        uninstall_wasmedge_tensorflow
        wasmedge_checks wasmedge-tensorflow \
            wasmedgec-tensorflow \
            wasmedge-tensorflow-lite
    elif [ "$EXT" = "none" ]; then
        echo "No extensions to be installed"
    else
        echo "Invalid extension"
    fi

}

main "$@"
