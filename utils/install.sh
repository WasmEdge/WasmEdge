#!/bin/bash
set -e

RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
YELLOW=$'\e[0;33m'
NC=$'\e[0m' # No Color
PYTHON_EXECUTABLE="${PYTHON_EXECUTABLE:=}"

if ! command -v git &>/dev/null; then
    echo "${RED}Please install git${NC}"
    exit 1
fi

main() {
    if [ "$PYTHON_EXECUTABLE" = "" ]; then
        if command -v python3 &>/dev/null; then
            PYTHON_EXECUTABLE="$(which python3)"
        elif command -v python2 &>/dev/null; then
            PYTHON_EXECUTABLE="$(which python2)"
        elif command -v python &>/dev/null; then
            PYTHON_EXECUTABLE="$(which python)"
        else
            echo "${RED}Please install python or provide python path:"
            echo "PYTHON_EXECUTABLE=<path> install.sh${NC}"
            exit 1
        fi
        echo "${YELLOW}Using Python: $PYTHON_EXECUTABLE ${NC}"
    else
        echo "${GREEN}Using Python: $PYTHON_EXECUTABLE ${NC}"
    fi

    curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.py | "$PYTHON_EXECUTABLE" - "$@"
}

main "$@"
