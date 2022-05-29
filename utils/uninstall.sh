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
        if command -v ldconfig &>/dev/null; then
            ldconfig "$IPATH/lib"
        elif command -v update_dyld_shared_cache &>/dev/null; then
            update_dyld_shared_cache
        fi
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

    -V,             --verbose                   Run script in verbose mode.
                                                    Will print out each step 
                                                    of execution.

    Example:
    ./$0 -p $IPATH --verbose
    
    Or
    ./$0 --verbose
    
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
    local _COUNT_
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
            _COUNT_=$((_COUNT_ + 1))
            echo "${line#"#"}"
        fi
    done <"$IPATH/env"
    if [ -f "$IPATH/env" ]; then
        _COUNT_=$((_COUNT_ + 1))
        echo "$IPATH/env"
    fi
    if [ $_COUNT_ -lt 2 ]; then
        echo "_ERROR_ : Found $_COUNT_ file(s) only"
    fi
}

remove_parsed() {
    if [ -f "$IPATH/env" ]; then
        IFS=$'\n'
        ask_remove $(parse_env)
        if [[ "$IPATH" =~ ".wasmedge" ]]; then
            ask_remove $(find "$IPATH" -depth -type d -empty -print)
            ask_remove $(find "$IPATH" -depth -type d -empty -print)
            if [ -z "$(ls -A "$IPATH")" ]; then
                ask_remove "$IPATH"
            fi
        fi
    else
        echo "${RED}env file not found${NC}"
    fi
}

detect_bin_path() {
    set +e
    _path=$(which "$1")
    if [ "$_path" = "" ] || [ "$SPECIFIED" -eq 1 ]; then
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
                if [ "$var" = "" ] || [[ "$var" == "_ERROR_"* ]]; then
                    echo "${RED}Error parsing file:$var${NC}"
                    exit 1
                fi
                echo "$var"
            done
            read -rp "Please answer [Y/N | y/n]" yn
            case $yn in
            [Yy]*)
                for var in "${libs[@]}"; do
                    echo "Removing $var"
                    _rm "$var"
                done
                _ldconfig
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
            if [ "$var" = "" ] || [ "$var" = "_ERROR_" ]; then
                echo "${RED}Error parsing file${NC}"
                exit 1
            fi
            echo "Removing $var"
            _rm "$var"
        done
        _ldconfig
    fi
}

main() {

    # getopt is in the util-linux package,
    # it'll probably be fine, but it's of course a good thing to keep in mind.
    SPECIFIED=0
    local OPTIND
    while getopts "qhp:V-:" OPT; do
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
        h | help)
            usage
            trap - EXIT
            exit 0
            ;;
        V | verbose)
            VERBOSE=1
            ;;
        p | path)
            IPATH="${OPTARG}"
            SPECIFIED=1
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

    local _shell_ _shell_rc line_num
    _shell_="${SHELL#${SHELL%/*}/}"
    _shell_rc=".""$_shell_""rc"

    [[ -f "${__HOME__}/${_shell_rc}" ]] && line_num="$(grep -n ". \"${IPATH}/env\"" "${__HOME__}/${_shell_rc}" | cut -d : -f 1)" &&
        [ "$line_num" != "" ] && sed -i.wasmedge_backup -e "${line_num}"'d' "${__HOME__}/${_shell_rc}"
    [[ -f "${__HOME__}/.profile" ]] && line_num="$(grep -n ". \"${IPATH}/env\"" "${__HOME__}/.profile" | cut -d : -f 1)" &&
        [[ "$line_num" != "" ]] && sed -i.wasmedge_backup -e "${line_num}"'d' "${__HOME__}/.profile"
    [[ -f "${__HOME__}/.bash_profile" ]] && line_num="$(grep -n ". \"${IPATH}/env\"" "${__HOME__}/.bash_profile" | cut -d : -f 1)" &&
        [[ "$line_num" != "" ]] && sed -i.wasmedge_backup -e "${line_num}"'d' "${__HOME__}/.bash_profile"

    exit 0
}

main "$@"
