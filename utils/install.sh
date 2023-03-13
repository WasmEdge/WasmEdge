#!/bin/bash
set -e

RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
YELLOW=$'\e[0;33m'
NC=$'\e[0m' # No Color
PYTHON_EXECUTABLE="${PYTHON_EXECUTABLE:=}"
INSTALL_PY_URL="${INSTALL_PY_URL:=}"

if ! command -v git &>/dev/null; then
    echo "${RED}Please install git${NC}"
    exit 1
fi

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

    if [ "$INSTALL_PY_URL" = "" ]; then
        INSTALL_PY_URL="https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.py"
    fi

    if command -v curl &>/dev/null; then
        if curl --output /dev/null --silent --head --fail "$INSTALL_PY_URL"; then
            curl -sSf "$INSTALL_PY_URL" | "$PYTHON_EXECUTABLE" - "$@"
        else
            echo "${RED}$INSTALL_PY_URL not reachable${NC}"
        fi

    elif command -v wget &>/dev/null; then
        if wget -q --method=HEAD "$INSTALL_PY_URL"; then
            wget -qO- "$INSTALL_PY_URL" | "$PYTHON_EXECUTABLE" - "$@"
        else
            echo "${RED}$INSTALL_PY_URL not reachable{NC}"
        fi
    else
        echo "${RED}curl or wget could not be found${NC}"
        exit 1
    fi

}

main "$@"
