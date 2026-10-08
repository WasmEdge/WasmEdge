#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors
set -euo pipefail
cd "$(dirname "$0")"
CLANG=${CLANG:-clang}
if ! "$CLANG" --version | grep -q 'clang version 21\.'; then
  echo "build.sh needs clang 21 (set CLANG=...)" >&2
  exit 1
fi
WASM_LD=$("$CLANG" -print-prog-name=wasm-ld)
SRC=$PWD
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT
FLAGS=(--target=wasm32 -nostdlib -fdebug-prefix-map="$PWD"=/wasmedge/debuginfo
  -Wl,--no-entry -Wl,--export-dynamic)
"$CLANG" --target=wasm32 -O0 -c dwarf_plain.c -o "$OUT/dwarf_plain.o"
"$CLANG" "${FLAGS[@]}" -g -O0 dwarf_basic.c "$OUT/dwarf_plain.o" \
  -o "$OUT/dwarf_basic_O0.wasm"
"$CLANG" "${FLAGS[@]}" -g -O2 dwarf_basic.c "$OUT/dwarf_plain.o" \
  -o "$OUT/dwarf_basic_O2.wasm"
"$CLANG" "${FLAGS[@]}" -O0 dwarf_basic.c -o "$OUT/dwarf_basic_nodebug.wasm"
"$CLANG" "${FLAGS[@]}" -g -O1 dwarf_nomem.c -o "$OUT/dwarf_nomem_O1_mem.wasm"
python3 strip_memory.py "$OUT/dwarf_nomem_O1_mem.wasm" \
  "$OUT/dwarf_nomem_O0.wasm"
wasm-validate "$OUT/dwarf_nomem_O0.wasm"
rm -f "$OUT/dwarf_nomem_O1_mem.wasm"
"$CLANG" "${FLAGS[@]}" -g -Og dwarf_scope.c -o "$OUT/dwarf_scope_Og.wasm"
rustc --target wasm32-unknown-unknown -g -C opt-level=0 \
  -C overflow-checks=off -C debug-assertions=off -C panic=abort \
  --crate-type cdylib --emit=obj \
  --remap-path-prefix "$SRC"=/wasmedge/debuginfo \
  --remap-path-prefix "$(rustc --print sysroot)"=/rustc dwarf_enum.rs \
  -o "$OUT/dwarf_enum.o"
"$WASM_LD" --no-entry --export=pick --export-dynamic "$OUT/dwarf_enum.o" \
  -o "$OUT/dwarf_enum_O0.wasm"
for W in "$OUT"/*.wasm; do
  python3 hex_tool.py --to-hex "$W" "$SRC/$(basename "$W").hex"
done
