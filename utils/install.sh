#!/bin/bash
set -e

RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
YELLOW=$'\e[0;33m'
NC=$'\e[0m' # No Color
PYTHON_EXECUTABLE="${PYTHON_EXECUTABLE:=}"
INSTALL_PY_URL="${INSTALL_PY_URL:=}"
INSTALL_PY_PATH="${INSTALL_PY_PATH:=}"

if ! command -v git &>/dev/null; then
    echo "${RED}Please install git${NC}"
    exit 1
fi

download_installer() {
    if command -v curl &>/dev/null; then
        curl --location --output /dev/null --silent --head --fail "$INSTALL_PY_URL" &&
            curl --location -sSf -o "$1" "$INSTALL_PY_URL"
    else
        wget -q --method=HEAD "$INSTALL_PY_URL" &&
            wget -qO "$1" "$INSTALL_PY_URL"
    fi
}

main() {
    if [ "$PYTHON_EXECUTABLE" = "" ]; then
        if ! command -v which &>/dev/null; then
            echo "${RED}Please install python or provide python path:"
            echo "PYTHON_EXECUTABLE=<path> install.sh"
            echo "Please install which${NC}"
            exit 1
        fi
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

    if [ "$INSTALL_PY_PATH" != "" ]; then
        # If a local installer script is given, then use it directly
        "$PYTHON_EXECUTABLE" "$INSTALL_PY_PATH" "$@"
        exit 0
    fi

    if [ "$INSTALL_PY_URL" = "" ]; then
        INSTALL_PY_URL="https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.py"
    fi

    if ! command -v curl &>/dev/null && ! command -v wget &>/dev/null; then
        echo "${RED}curl or wget could not be found${NC}" >&2
        exit 1
    fi

    installer=$(mktemp "${TMPDIR:-/tmp}/wasmedge-install.XXXXXXXXXX")
    trap 'rm -f -- "$installer"' EXIT

    if ! download_installer "$installer" || [[ ! -s "$installer" ]]; then
        echo "${RED}Could not download $INSTALL_PY_URL${NC}" >&2
        exit 1
    fi

    "$PYTHON_EXECUTABLE" - "$@" < "$installer"
}

main "$@"
