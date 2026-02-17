#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2026 Second State INC

set -euo pipefail

ITERATION_COUNT="${FUZZ_ITERATION_COUNT:-100}"
BINARY="${1:?Usage: $0 <componentFuzzTest-binary>}"

if ! command -v wasm-tools &> /dev/null; then
  echo "Error: wasm-tools is not installed." >&2
  echo "Install with: cargo install wasm-tools" >&2
  exit 1
fi

TMPFILE=$(mktemp --suffix=.wasm)
trap 'rm -f "$TMPFILE"' EXIT

echo "Running component fuzz test: ${ITERATION_COUNT} iterations"

for i in $(seq 1 "$ITERATION_COUNT"); do
  head -c 100 /dev/urandom | wasm-tools wit-smith --futures -o "$TMPFILE"

  if ! "$BINARY" "$TMPFILE"; then
    echo "FAIL at iteration ${i}/${ITERATION_COUNT}"
    echo "--- WAT dump of failing wasm ---"
    wasm-tools parse -t "$TMPFILE" || true
    echo "--- end WAT dump ---"
    exit 1
  fi
done

echo "PASS: all ${ITERATION_COUNT} iterations succeeded"
