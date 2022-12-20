#!/bin/bash

RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
YELLOW=$'\e[0;33m'
NC=$'\e[0m' # No Color

test_diff_env() {
    local _path_=$1
    local _common_args_=$2
    echo "Testing path: $_path_ args: $_common_args_"
    bash ./utils/install.sh.old -p "$_path_" $_common_args_
    cp "$_path_"/env "$HOME"/env.old
    INSTALL_PY_URL="https://raw.githubusercontent.com/SAtacker/WasmEdge/fix_python_installer/utils/install.py" bash ./utils/install.sh -p "$_path_" $_common_args_
    cp "$_path_"/env "$HOME"/env
    diff -u \
        <(sed '1,/Please/d' "$HOME"/env.old | sed -e 's/\/\//\//g' |
            sed 's/\/$//' | sed -e 's/lib\/wasmedge$/lib/g' | sort |
            while read -r line; do [ -f "${line/##/}" ] && echo "$line"; done;) \
        <(sed '1,/Please/d' "$HOME"/env | sed '\/bin$/d' |
            sort | while read -r line; do [ -f "${line/##/}" ] &&
                [[ ! $line =~ (((tensorflow|framework)\.so\.[0-9]\.[0-9]$)|((tensorflow|framework)\.[0-9]\.[0-9]\.dylib$)) ]] &&
                echo "$line"; done;)

    error=$?
    if [ $error -eq 0 ]; then
        echo "${GREEN}All Safe${NC}"
    elif [ $error -eq 1 ]; then
        echo "================================================================================"
        echo "${RED}Raw Old:"
        cat "$HOME"/env.old
        echo "================================================================================"
        echo "Raw New:"
        cat "$HOME"/env
        echo "${NC}"
        exit 1
    else
        echo "${YELLOW}There was something wrong with the diff command${NC}"
        exit 1
    fi
}

test_diff_env "$HOME"/.wasmedge
test_diff_env "$HOME"/new_folder
test_diff_env /usr/local
test_diff_env /usr

test_diff_env "$HOME"/.wasmedge "-e all"
test_diff_env "$HOME"/new_folder "-e all"
test_diff_env /usr/local "-e all"
test_diff_env /usr "-e all"
