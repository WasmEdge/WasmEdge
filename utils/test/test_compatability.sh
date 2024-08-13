#!/usr/bin/env bash
GIT_SHA=$(git rev-parse --short --verify HEAD)

ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." &> /dev/null && pwd)
BUILD_DIR=${BUILD_DIR:-"/tmp/WasmEdge-${GIT_SHA}"}

SRC_IMAGE=wasmedge/wasmedge:manylinux_2_28_x86_64-plugins-deps
TGT_IMAGE=wasmedge/wasmedge:ubuntu-build-gcc

V_ROOT_DIR=/root/WasmEdge
V_BUILD_DIR=/root/build

PLUGIN_DEF_FILE=${ROOT_DIR}/.github/workflows/matrix-extensions.json

pq() {
    if [[ $# -eq 1 ]]; then
        jq -r ".list.\"$1\"[]" "${PLUGIN_DEF_FILE}"
    elif [[ $# -eq 2 ]]; then
        jq -r ".detail.\"$1\".\"$2\"" "${PLUGIN_DEF_FILE}"
    else
        return 3
    fi
}

build_plugin() {
    docker run -it --rm \
        -e "BUILD_DIR=${V_BUILD_DIR}" \
        -e "WASMEDGE_OPTIONS=$(pq "$1" options)" \
        -e "WASMEDGE_TARGET=$(pq "$1" testBin)" \
        -v "${ROOT_DIR}:${V_ROOT_DIR}" \
        -v "${BUILD_DIR}:${V_BUILD_DIR}" \
        -w "${V_ROOT_DIR}" \
        "${SRC_IMAGE}" \
        bash "${V_ROOT_DIR}/utils/test/entry_build.sh"
}

test_plugin() {
    docker run -it --rm \
        -e "WASMEDGE_PLUGIN_DIR=$(pq "$1" dir)" \
        -e "WASMEDGE_TARGET=$(pq "$1" testBin)" \
        -v "${ROOT_DIR}:${V_ROOT_DIR}" \
        -v "${BUILD_DIR}:${V_BUILD_DIR}" \
        -w "${V_BUILD_DIR}" \
        "${TGT_IMAGE}" \
        bash "${V_ROOT_DIR}/utils/test/entry_test.sh"
}

cleanup() {
    docker run -it --rm \
        -v "${BUILD_DIR}:${V_BUILD_DIR}" \
        -w "${V_BUILD_DIR}" \
        "${SRC_IMAGE}" \
        bash -c 'rm -rf * .*'
    rmdir ${BUILD_DIR}
}

trap 'cleanup' EXIT
mkdir -p ${BUILD_DIR}

for PLUGIN in $(pq manylinux_2_28_x86_64); do
    echo "plugin: ${PLUGIN}"
    time build_plugin "${PLUGIN}" 1>&2
    test_plugin "${PLUGIN}" 1>&2
    echo "exit: $?"
done
