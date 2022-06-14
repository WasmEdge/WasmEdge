#! /usr/bin/env bash

# Usage: $0 clang-format(version >= 10.0)
# $ bash clang-format.sh `which clang-format`

lint() {
    local targets="include lib tools plugins examples"
    local clang_format="${1}"

    if [ "$#" -ne 1 ]; then
        echo "please provide clang-format command. Usage ${0} `which clang-format`"
        exit 1
    fi

    if [ ! -f "${clang_format}" ]; then
        echo "clang-format not found. Please install clang-format first"
        exit 1
    fi

    find ${targets} -type f -iname *.[ch] -o -iname *.cpp -o -iname *.[ch]xx \
        | xargs -n1 ${clang_format} -i -style=file -Werror --dry-run

    exit $?
}

lint $@
