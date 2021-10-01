#!/bin/bash
set -e

RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
YELLOW=$'\e[0;33m'
NC=$'\e[0m' # No Color
PERM_ROOT=1
BLOCK_BEGIN="# Please"

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
    Long options should be assingned with '='

    -h,             --help                      Display help
    -q,             --quick                     Uninstall everything without
                                                asking
    -p,             --path=[/usr/local]         Prefix / Path to install

    -v,             --version=VERSION           Set and Download specific 
                                                    version of WasmEdge
                        
                    --tf-version=VERSION_TF
                    --tf-deps-version=VERSION_TF_DEPS
                    --tf-tools-version=VERSION_TF_TOOLS
                    --image-version=VERSION_IM
                    --image-deps-version=VERSION_IM_DEPS

    -e,             --extension=[tf|image|all|none]  
                                                    Enable extension support 
                                                    i.e Tensorflow (tf) 
                                                        or Image (image)

    -V,             --verbose                   Run script in verbose mode.
                                                    Will print out each step 
                                                    of execution.

    Example:
    ./$0 -p $IPATH -e all -v $VERSION --verbose
    
    Or
    ./$0 -p $IPATH --extension=all --path=/usr/local --verbose
    
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

parse_env() {
    local begin
    begin=0
    while IFS= read -r line; do
        if [ ! "${line:0:1}" = "#" ]; then
            continue
        else
            if [[ "$line" =~ $BLOCK_BEGIN ]]; then
                begin=1
            fi
        fi
        if [ $begin -eq 1 ] && [[ ! "$line" =~ $BLOCK_BEGIN ]]; then
            echo "${line#"#"}"
        fi
    done <"$IPATH/env"
    if [ -f "$IPATH/env" ]; then
        echo "$IPATH/env"
    fi
}

remove_parsed() {
    if [ -f "$IPATH/env" ]; then
        IFS=$'\n'
        ask_remove $(parse_env)
        if [[ "$IPATH" =~ ".wasmedge" ]]; then
            ask_remove $(find "$IPATH" -type d -empty -print)
            if [ -z "$(ls -A "$IPATH")" ]; then
                ask_remove "$IPATH"
            fi
        fi
        exit 0
    fi
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
            remove_parsed
        fi
    else
        IPATH=${_path%"/bin/$1"}
        echo "${GREEN}Installation path found at $IPATH${NC}"
        remove_parsed
    fi
    set -e
}

_rm() {
    local var=$1
    if [ -f "$var" ] || [ -h "$var" ]; then
        rm -f "$var"
    elif [ -d "$var" ]; then
        rmdir "$var" 2>/dev/null
    fi
}

ask_remove() {
    local libs=("$@")
    if [ $ASK == 1 ]; then
        while true; do
            echo "Do you wish to uninstall the following?"
            for var in "${libs[@]}"; do
                echo "$var"
            done
            read -p "Please answer [Y/N | y/n]" yn
            case $yn in
            [Yy]*)
                for var in "${libs[@]}"; do
                    echo "Removing $var"
                    _rm "$var"
                done
                ldconfig
                break
                ;;
            [Nn]*)
                echo "Aborted Uninstalling"
                break
                ;;
            *) echo "Please answer [Y/N | y/n]" ;;
            esac
        done
    else
        for var in "${libs[@]}"; do
            echo "Removing $var"
            _rm "$var"
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

    default=0

    local OPTIND
    while getopts "qe:hp:v:V-:" OPT; do
        # support long options: https://stackoverflow.com/a/28466267/519360
        if [ "$OPT" = "-" ]; then   # long option: reformulate OPT and OPTARG
            OPT="${OPTARG%%=*}"     # extract long option name
            OPTARG="${OPTARG#$OPT}" # extract long option argument (may be empty)
            OPTARG="${OPTARG#=}"    # if long option argument, remove assigning `=`
        fi
        case "$OPT" in
        q | quick)
            ASK=0
            ;;
        e | extension)
            EXT="${OPTARG}"
            ;;
        h | help)
            usage
            trap - EXIT
            exit 0
            ;;
        v | version)
            VERSION="${OPTARG}"
            ;;
        V | verbose)
            VERBOSE=1
            ;;
        p | path)
            IPATH="${OPTARG}"
            default=1
            ;;
        tf-version)
            VERSION_TF="${OPTARG}"
            ;;
        tf-deps-version)
            VERSION_TF_DEPS="${OPTARG}"
            ;;
        tf-tools-version)
            VERSION_TF_TOOLS="${OPTARG}"
            ;;
        image-version)
            VERSION_IM="${OPTARG}"
            ;;
        image-deps-version)
            VERSION_IM_DEPS="$OPTARG"
            ;;
        ?)
            exit 2
            ;;
        ??*)
            echo "${RED}Illegal option${NC}"
            exit 1
            ;;
        *)
            echo "Internal error!"
            exit 1
            ;;
        esac
    done

    shift $((OPTIND - 1)) # remove parsed options and args from $@ list

    if [ ! $VERBOSE == 0 ]; then
        echo "Verbose Mode"
        set -xv
    fi

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
        echo "No extensions to be uninstalled"
    else
        echo "Invalid extension"
    fi

}

main "$@"
