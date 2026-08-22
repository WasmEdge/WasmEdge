#!/usr/bin/env bash

set -euo pipefail

if [[ $# -lt 3 ]]; then
  echo "Usage: $0 <wasmedgec> <input.wasm> <output-dir> [compiler-options...]" >&2
  exit 1
fi

canonicalize() {
  local path=$1
  local directory
  local filename
  directory=$(dirname "$path")
  filename=$(basename "$path")
  printf '%s/%s\n' "$(cd "$directory" && pwd -P)" "$filename"
}

if [[ $1 == */* ]]; then
  if [[ ! -x $1 ]]; then
    echo "Error: wasmedgec is not executable: $1" >&2
    exit 1
  fi
  wasmedgec=$(canonicalize "$1")
elif ! wasmedgec=$(command -v -- "$1"); then
  echo "Error: wasmedgec command not found: $1" >&2
  exit 1
fi

if [[ ! -f $2 || ! -r $2 ]]; then
  echo "Error: input is not a readable file: $2" >&2
  exit 1
fi
input=$(canonicalize "$2")
output_dir=$3
shift 3
compiler_options=("$@")

if ! llvm_readobj=$(command -v llvm-readobj); then
  echo "Error: llvm-readobj command not found" >&2
  exit 1
fi
if ! llvm_objdump=$(command -v llvm-objdump); then
  echo "Error: llvm-objdump command not found" >&2
  exit 1
fi

if [[ -e $output_dir && ! -d $output_dir ]]; then
  echo "Error: output path is not a directory: $output_dir" >&2
  exit 1
fi
if [[ -d $output_dir ]]; then
  shopt -s nullglob dotglob
  output_entries=("$output_dir"/*)
  shopt -u nullglob dotglob
  if (( ${#output_entries[@]} != 0 )); then
    echo "Error: output directory is not empty: $output_dir" >&2
    exit 1
  fi
fi

mkdir -p "$output_dir"
cd "$output_dir"

object_report=$(mktemp object.txt.tmp.XXXXXX)
disassembly_report=$(mktemp disassembly.txt.tmp.XXXXXX)
cleanup() {
  rm -f -- "$object_report" "$disassembly_report"
}
trap cleanup EXIT

"$wasmedgec" --dump "${compiler_options[@]}" "$input" output.aot.wasm
"$llvm_readobj" --file-headers --sections --symbols --relocations wasm.o > "$object_report"
"$llvm_objdump" -dr wasm.o > "$disassembly_report"
mv -- "$object_report" object.txt
mv -- "$disassembly_report" disassembly.txt
