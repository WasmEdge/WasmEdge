#!/bin/bash

set -e

LIB_PATH=$1 
WHITELIST="WasmEdge/lib/api/whitelist.symbols"

if [[ "$OSTYPE" == "linux-gnu"* || "$OSTYPE" == "darwin"* ]]; then
    nm -D --defined-only "$LIB_PATH" | awk '{print $3}' | sort > extracted_symbols.txt
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    dumpbin /EXPORTS "$LIB_PATH" | awk '{print $4}' | sort > extracted_symbols.txt
else
    echo "Unsupported OS"
    exit 1
fi

diff -u "$WHITELIST" extracted_symbols.txt || {
    echo "Error: Unexpected symbols detected!"
    exit 1
}

echo "Symbol check passed."
