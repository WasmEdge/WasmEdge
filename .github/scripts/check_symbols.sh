#!/bin/bash

# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2024 Second State INC

# Script to check that libwasmedge doesn't expose unnecessary symbols
# This script compares the exported symbols against a whitelist to ensure
# only intended API symbols are exposed.

set -euo pipefail  # Exit on error, undefined vars, pipe failures

# Default paths
SYMBOLS_PATH=${SYMBOLS_PATH:-"$(pwd)"}
WHITELIST_FILE="$SYMBOLS_PATH/lib/api/whitelist.symbols"

# Detect OS and set library path accordingly
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    LIB_PATH="$SYMBOLS_PATH/build/lib/api/libwasmedge.so"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    LIB_PATH="$SYMBOLS_PATH/build/lib/api/libwasmedge.dylib"
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    LIB_PATH="$SYMBOLS_PATH/build/lib/api/wasmedge.dll"
else
    echo "Unsupported OS: $OSTYPE"
    exit 1
fi

echo "Checking symbol exposure for libwasmedge..."
echo "Library path: $LIB_PATH"
echo "Whitelist file: $WHITELIST_FILE"

# Check if library exists
if [ ! -f "$LIB_PATH" ]; then
    echo "Error: Library not found at $LIB_PATH"
    echo "Make sure you have built WasmEdge first with: cmake --build build"
    exit 1
fi

# Check if whitelist exists
if [ ! -f "$WHITELIST_FILE" ]; then
    echo "Error: Whitelist file not found at $WHITELIST_FILE"
    exit 1
fi

# Verify whitelist is not empty
if [ ! -s "$WHITELIST_FILE" ]; then
    echo "Error: Whitelist file is empty: $WHITELIST_FILE"
    exit 1
fi

# Create temporary directory for processing
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

# Extract symbols from library
echo "Extracting symbols from library..."
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    # Windows: Use dumpbin to extract exports
    if ! command -v dumpbin >/dev/null 2>&1; then
        echo "Error: dumpbin not found. Make sure Visual Studio tools are in PATH"
        exit 1
    fi
    dumpbin //EXPORTS "$LIB_PATH" | awk '/^[[:space:]]*[0-9]+[[:space:]]+[0-9A-Fa-f]+[[:space:]]+[0-9A-Fa-f]+[[:space:]]+/ {print $4}' | grep -E "^WasmEdge" | sort > "$TEMP_DIR/extracted.symbols"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS: Use nm to extract symbols
    if ! command -v nm >/dev/null 2>&1; then
        echo "Error: nm not found. Make sure Xcode Command Line Tools are installed"
        exit 1
    fi
    nm -D "$LIB_PATH" | awk '/^[0-9a-fA-F]+ [TBDW] / {print $3}' | grep -E "^WasmEdge" | sort > "$TEMP_DIR/extracted.symbols"
else
    # Linux: Use nm to extract symbols
    if ! command -v nm >/dev/null 2>&1; then
        echo "Error: nm not found. Install binutils package"
        exit 1
    fi
    nm -D --defined-only "$LIB_PATH" | awk '/^[0-9a-fA-F]+ [TBDW] / {print $3}' | grep -E "^WasmEdge" | sort > "$TEMP_DIR/extracted.symbols"
fi

# Verify that we extracted some symbols
if [ ! -s "$TEMP_DIR/extracted.symbols" ]; then
    echo "Error: No WasmEdge symbols found in library. This might indicate:"
    echo "  1. Library was not built correctly"
    echo "  2. Symbol extraction failed"
    echo "  3. All symbols are hidden (check build configuration)"
    exit 1
fi

# Sort whitelist for comparison and convert line endings
sort "$WHITELIST_FILE" | tr -d '\r' > "$TEMP_DIR/whitelist_sorted.symbols"

# Check for unexpected symbols (in library but not in whitelist)
comm -23 "$TEMP_DIR/extracted.symbols" "$TEMP_DIR/whitelist_sorted.symbols" > "$TEMP_DIR/unexpected.symbols"

# Check for missing symbols (in whitelist but not in library)
comm -13 "$TEMP_DIR/extracted.symbols" "$TEMP_DIR/whitelist_sorted.symbols" > "$TEMP_DIR/missing.symbols"

# Report results
EXTRACTED_COUNT=$(wc -l < "$TEMP_DIR/extracted.symbols")
WHITELIST_COUNT=$(wc -l < "$TEMP_DIR/whitelist_sorted.symbols")
UNEXPECTED_COUNT=$(wc -l < "$TEMP_DIR/unexpected.symbols")
MISSING_COUNT=$(wc -l < "$TEMP_DIR/missing.symbols")

echo "Symbol analysis results:"
echo "  Extracted symbols: $EXTRACTED_COUNT"
echo "  Whitelisted symbols: $WHITELIST_COUNT"
echo "  Unexpected symbols: $UNEXPECTED_COUNT"
echo "  Missing symbols: $MISSING_COUNT"

# Show unexpected symbols if any
if [ "$UNEXPECTED_COUNT" -gt 0 ]; then
    echo
    echo "❌ ERROR: Found unexpected symbols exported by libwasmedge:"
    cat "$TEMP_DIR/unexpected.symbols"
    echo
    echo "These symbols should either be:"
    echo "1. Added to the whitelist if they are intended public API"
    echo "2. Made static or marked with hidden visibility if they are internal"
    echo
    exit 1
fi

# Show missing symbols if any (warning only)
if [ "$MISSING_COUNT" -gt 0 ]; then
    echo
    echo "⚠️  WARNING: Some whitelisted symbols were not found in the library:"
    cat "$TEMP_DIR/missing.symbols"
    echo
    echo "This might indicate:"
    echo "1. Symbols removed from the API (update whitelist)"
    echo "2. Conditional compilation excluding some symbols"
    echo "3. Platform-specific symbols not available on this build"
    echo
fi

echo "✅ Symbol exposure check passed - no unexpected symbols found!"
exit 0