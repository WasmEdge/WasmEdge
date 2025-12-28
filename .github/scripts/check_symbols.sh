#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2025 Second State INC

set -euo pipefail

VERBOSE=false
for arg in "$@"; do
    case $arg in
        --verbose|-v)
            VERBOSE=true
            ;;
        --help|-h)
            echo "Usage: $0 [--verbose|-v] [--help|-h]"
            echo "  --verbose, -v    Enable verbose debug output"
            echo "  --help, -h       Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
YELLOW=$'\e[0;33m'
CYAN=$'\e[0;36m'
NC=$'\e[0m'

info() {
    printf '%s%s%s: %s\n\n' "$GREEN" "Info" "$NC" "$1"
}

warn() {
    printf '%s%s%s: %s\n\n' "$YELLOW" "Warn" "$NC" "$1"
}

error() {
    printf '%s%s%s: %s\n\n' "$RED" "Error" "$NC" "$1" 1>&2
}

debug() {
    if [ "$VERBOSE" = true ]; then
        printf '%s%s%s: %s\n' "$CYAN" "Debug" "$NC" "$1" 1>&2
    fi
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${PROJECT_ROOT:-"${SCRIPT_DIR%/.github/scripts}"}"
WHITELIST_FILE="$SCRIPT_DIR/whitelist.symbols"

if [ -n "${LIB_PATH:-}" ]; then
    debug "Using user-specified LIB_PATH: $LIB_PATH"
else
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        LIB_PATH="$PROJECT_ROOT/build/lib/api/libwasmedge.so"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        LIB_PATH="$PROJECT_ROOT/build/lib/api/libwasmedge.dylib"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        LIB_PATH="$PROJECT_ROOT/build/lib/api/wasmedge.dll"
    else
        error "Unsupported OS: $OSTYPE"
        exit 1
    fi
fi

info "Checking symbol exposure for libwasmedge..."
debug "Project root: $PROJECT_ROOT"
debug "Library path: $LIB_PATH"
debug "Whitelist file: $WHITELIST_FILE"

if [ ! -f "$LIB_PATH" ]; then
    if [[ "$OSTYPE" == "linux-gnu"* ]] || [[ "$OSTYPE" == "darwin"* ]]; then
        STATIC_LIB="$PROJECT_ROOT/build/lib/api/libwasmedge.a"
    else
        STATIC_LIB="$PROJECT_ROOT/build/lib/api/wasmedge.lib"
    fi

    if [ -f "$STATIC_LIB" ]; then
        info "Skipping symbol exposure check for static-only build."
        exit 0
    fi

    error "Library not found at $LIB_PATH"
    echo "Make sure you have built WasmEdge first with: cmake --build build" 1>&2
    exit 1
fi

if [ ! -f "$WHITELIST_FILE" ]; then
    error "Whitelist file not found at $WHITELIST_FILE"
    exit 1
fi

TEMP_DIR=$(mktemp -d)
trap 'rm -rf "$TEMP_DIR"' EXIT

info "Extracting symbols from library..."

if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    # //EXPORTS instead of /EXPORTS due to shell escaping in CI
    if command -v dumpbin >/dev/null 2>&1; then
        dumpbin //EXPORTS "$LIB_PATH" 2>/dev/null | \
            awk '/^[[:space:]]*[0-9]+[[:space:]]+[0-9A-Fa-f]+[[:space:]]+[0-9A-Fa-f]+[[:space:]]+/ {print $4}' | \
            grep -E "^WasmEdge" | sort > "$TEMP_DIR/extracted.symbols"
    elif command -v objdump >/dev/null 2>&1; then
        info "dumpbin not found, using objdump as fallback..."
        objdump -p "$LIB_PATH" 2>/dev/null | \
            grep -E "^\s*\[[[:space:]]*[0-9]+\]" | \
            grep "WasmEdge" | awk '{print $NF}' | sort > "$TEMP_DIR/extracted.symbols"
    else
        error "Neither dumpbin nor objdump found. Make sure Visual Studio tools are in PATH or MinGW is installed"
        exit 1
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    if ! command -v nm >/dev/null 2>&1; then
        error "nm not found. Make sure Xcode Command Line Tools are installed"
        exit 1
    fi

    debug "Extracting symbols from macOS dylib..."
    # macOS symbols have underscore prefix, so we strip it
    nm -g "$LIB_PATH" 2>/dev/null | \
        awk '/^[0-9a-fA-F]+ [TBDWS] _WasmEdge/ {print substr($3,2)}' | \
        sort > "$TEMP_DIR/extracted.symbols"
else
    if ! command -v nm >/dev/null 2>&1; then
        error "nm not found. Install binutils package"
        exit 1
    fi

    nm -D --defined-only "$LIB_PATH" 2>/dev/null | \
        awk '/^[0-9a-fA-F]+ [TBDW] / {print $3}' | \
        grep -E "^WasmEdge" | sort > "$TEMP_DIR/extracted.symbols"
fi

if [ ! -s "$TEMP_DIR/extracted.symbols" ]; then
    error "No WasmEdge symbols found in library. This might indicate:"
    echo "  1. Library was not built correctly" 1>&2
    echo "  2. Symbol extraction failed" 1>&2
    echo "  3. All symbols are hidden (check build configuration)" 1>&2
    exit 1
fi

sort "$WHITELIST_FILE" | tr -d '\r' > "$TEMP_DIR/whitelist_sorted.symbols"

grep -Fxv -f "$TEMP_DIR/whitelist_sorted.symbols" "$TEMP_DIR/extracted.symbols" \
    > "$TEMP_DIR/unexpected.symbols" 2>/dev/null || true

grep -Fxv -f "$TEMP_DIR/extracted.symbols" "$TEMP_DIR/whitelist_sorted.symbols" \
    > "$TEMP_DIR/missing.symbols" 2>/dev/null || true

EXTRACTED_COUNT=$(wc -l < "$TEMP_DIR/extracted.symbols" | tr -d ' ')
WHITELIST_COUNT=$(wc -l < "$TEMP_DIR/whitelist_sorted.symbols" | tr -d ' ')
UNEXPECTED_COUNT=$(wc -l < "$TEMP_DIR/unexpected.symbols" | tr -d ' ')
MISSING_COUNT=$(wc -l < "$TEMP_DIR/missing.symbols" | tr -d ' ')

debug "Extracted symbols: $EXTRACTED_COUNT"
debug "Whitelisted symbols: $WHITELIST_COUNT"
debug "Unexpected symbols: $UNEXPECTED_COUNT"
debug "Missing symbols: $MISSING_COUNT"

if [ "$UNEXPECTED_COUNT" -gt 0 ]; then
    echo "" 1>&2
    error "Found unexpected symbols exported by libwasmedge:"
    cat "$TEMP_DIR/unexpected.symbols" 1>&2
    echo "" 1>&2
    echo "These symbols should either be:" 1>&2
    echo "1. Added to the whitelist if they are intended public API" 1>&2
    echo "2. Made static or marked with hidden visibility if they are internal" 1>&2
    echo "" 1>&2
    exit 1
fi

if [ "$MISSING_COUNT" -gt 0 ]; then
    echo ""
    warn "Some whitelisted symbols were not found in the library:"
    cat "$TEMP_DIR/missing.symbols"
    echo ""
    echo "This might indicate:"
    echo "1. Symbols removed from the API (update whitelist)"
    echo "2. Conditional compilation excluding some symbols"
    echo "3. Platform-specific symbols not available on this build"
    echo ""
fi

info "Symbol exposure check passed - no unexpected symbols found!"
exit 0
