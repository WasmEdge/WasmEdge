#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

# Test WasmEdge WASI layer.
# The testcase is from https://github.com/khronosproject/wasi-test

set -Eeuo pipefail
trap cleanup SIGINT SIGTERM ERR EXIT

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)
current_dir=$(pwd -P)

usage() {
  cat <<EOF
Usage: $(basename "${BASH_SOURCE[0]}") [-h] [-v] [path_to_wasmedge_tools]

Run wasi-test testcases.

Available options:

-h, --help      Print this help and exit
EOF
  exit
}

cleanup() {
  trap - SIGINT SIGTERM ERR EXIT
  cd $current_dir
  msg "removing git repo"
  rm -Rf wasi-test
  return 0
}

msg() {
  echo >&2 -e "${1-}"
}

die() {
  local msg=$1
  local code=${2-1} # default exit status 1
  msg "$msg"
  exit "$code"
}

parse_params() {
  while :; do
    case "${1-}" in
    -h | --help) usage ;;
    -v | --verbose) set -x ;;
    -?*) die "Unknown option: $1" ;;
    *) break ;;
    esac
    shift
  done

  if ! command -v realpath &> /dev/null; then
    realpath() {
      readlink -f -- "$@"
    }
  fi

  local wasmedge_path=$(realpath "${1-}")
  msg "path = $wasmedge_path"
  if [[ x"$wasmedge_path" != x ]]; then
    export PATH="$wasmedge_path:$PATH"
  fi
  return 0
}

check_command() {
  if ! command -v "$1" &> /dev/null; then
    die "$1 not found!"
    exit 1
  fi
  return 0
}

parse_params "$@"
check_command git
check_command python3
check_command wasmedgec
check_command wasmedge

msg "Cloning git repo..."
git clone https://github.com/khronosproject/wasi-test.git --depth 1
cd wasi-test

msg "Applying patch..."
git apply "$script_dir"/0001-PATCH-Disable-other-tests-except-wasmedge.patch

if command -v cargo &> /dev/null; then
  msg "Building wasm files..."
  cargo build --release --target wasm32-wasi
else
  curl -L -O https://github.com/khronosproject/wasi-test-suite/archive/refs/heads/master.tar.gz
  mkdir -p target/wasm32-wasi
  tar -xf master.tar.gz -C target/wasm32-wasi
fi

msg "Running tests..."
python3 compat.py
