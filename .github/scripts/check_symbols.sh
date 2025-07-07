#!/bin/bash

# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2024 Second State INC

# Script to check that libwasmedge doesn't expose unnecessary symbols
# This script compares the exported symbols against a whitelist to ensure
# only intended API symbols are exposed.

set -euo pipefail  # Exit on error, undefined vars, pipe failures

# Parse command line arguments
VERBOSE=false
for arg in "$@"; do
    case $arg in
        --verbose|-v)
            VERBOSE=true
            shift
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

# Color definitions and utility functions
RED=$'\e[0;31m'
GREEN=$'\e[0;32m'
YELLOW=$'\e[0;33m'
NC=$'\e[0m' # No Color

info() {
	command printf '\e[0;32mInfo\e[0m: %s\n\n' "$1"
}

warn() {
	command printf '\e[0;33mWarn\e[0m: %s\n\n' "$1"
}

error() {
	command printf '\e[0;31mError\e[0m: %s\n\n' "$1" 1>&2
}

eprintf() {
	command printf '%s\n' "$1" 1>&2
}

debug() {
	if [ "$VERBOSE" = true ]; then
		command printf '\e[0;36mDebug\e[0m: %s\n' "$1" 1>&2
	fi
}

# Get script directory and derive paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SYMBOLS_PATH="${SYMBOLS_PATH:-"${SCRIPT_DIR%/.github/scripts}"}"
WHITELIST_FILE="$SCRIPT_DIR/whitelist.symbols"

# Detect OS and set library path accordingly
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    LIB_PATH="$SYMBOLS_PATH/build/lib/api/libwasmedge.so"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    LIB_PATH="$SYMBOLS_PATH/build/lib/api/libwasmedge.dylib"
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    LIB_PATH="$SYMBOLS_PATH/build/lib/api/wasmedge.dll"
else
    error "Unsupported OS: $OSTYPE"
    exit 1
fi

# Alternative library paths to check
ALT_PATHS=()
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    ALT_PATHS+=("$SYMBOLS_PATH/build/lib/libwasmedge.so")
elif [[ "$OSTYPE" == "darwin"* ]]; then
    ALT_PATHS+=("$SYMBOLS_PATH/build/lib/libwasmedge.dylib")
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    ALT_PATHS+=("$SYMBOLS_PATH/build/bin/wasmedge.dll")
    ALT_PATHS+=("$SYMBOLS_PATH/build/lib/wasmedge.dll")
fi

info "Checking symbol exposure for libwasmedge..."
debug "Working directory: $(pwd)"
debug "Library path: $LIB_PATH"
debug "Whitelist file: $WHITELIST_FILE"

# Check if library exists
if [ ! -f "$LIB_PATH" ]; then
    warn "Primary library not found at $LIB_PATH"
    info "Checking alternative locations..."
    
    FOUND=false
    for alt_path in "${ALT_PATHS[@]}"; do
        if [ -f "$alt_path" ]; then
            info "Found library at: $alt_path"
            LIB_PATH="$alt_path"
            FOUND=true
            break
        fi
    done
    
    if [ "$FOUND" = false ]; then
        error "Library not found in any of the expected locations:"
        eprintf "  Primary: $LIB_PATH"
        for alt_path in "${ALT_PATHS[@]}"; do
            eprintf "  Alternative: $alt_path"
        done
        eprintf ""
        info "Searching for library files in build directory..."
        if [[ "$OSTYPE" == "linux-gnu"* ]]; then
            SEARCH_RESULT=$(find "$SYMBOLS_PATH/build" -name "*wasmedge*.so" 2>/dev/null | head -5)
        elif [[ "$OSTYPE" == "darwin"* ]]; then
            SEARCH_RESULT=$(find "$SYMBOLS_PATH/build" -name "*wasmedge*.dylib" 2>/dev/null | head -5)
        elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
            SEARCH_RESULT=$(find "$SYMBOLS_PATH/build" -name "*wasmedge*.dll" 2>/dev/null | head -5)
        fi
        
        if [ -n "$SEARCH_RESULT" ]; then
            info "Found these WasmEdge library files:"
            eprintf "$SEARCH_RESULT"
            eprintf ""
            eprintf "Please update the script with the correct library path."
        else
            warn "No WasmEdge shared library files found in build directory."
            eprintf ""
            # Check if this is a static-only build
            if [[ "$OSTYPE" == "linux-gnu"* ]]; then
                STATIC_LIBS=$(find "$SYMBOLS_PATH/build" -name "*wasmedge*.a" 2>/dev/null | head -3)
            elif [[ "$OSTYPE" == "darwin"* ]]; then
                STATIC_LIBS=$(find "$SYMBOLS_PATH/build" -name "*wasmedge*.a" 2>/dev/null | head -3)
            elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
                STATIC_LIBS=$(find "$SYMBOLS_PATH/build" -name "*wasmedge*.lib" 2>/dev/null | head -3)
            fi
            
            if [ -n "$STATIC_LIBS" ]; then
                info "Found static libraries - this appears to be a static-only build:"
                eprintf "$STATIC_LIBS"
                eprintf ""
                info "✅ Skipping symbol exposure check for static-only build."
                exit 0
            else
                error "Make sure you have built WasmEdge first with: cmake --build build"
            fi
        fi
        exit 1
    fi
fi

# Check if whitelist exists
if [ ! -f "$WHITELIST_FILE" ]; then
    error "Whitelist file not found at $WHITELIST_FILE"
    exit 1
fi

# Create temporary directory for processing (cross-platform)
if command -v mktemp >/dev/null 2>&1; then
    TEMP_DIR=$(mktemp -d)
else
    # Fallback for Windows/systems without mktemp
    TEMP_DIR="/tmp/wasmedge_symbols_$$"
    mkdir -p "$TEMP_DIR"
fi
trap "rm -rf $TEMP_DIR" EXIT

# Extract symbols from library
info "Extracting symbols from library..."
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    # Windows: Use dumpbin to extract exports, fallback to objdump
    if command -v dumpbin >/dev/null 2>&1; then
        # Note: Using //EXPORTS instead of /EXPORTS due to shell escaping issues in certain environments
        # The double slash ensures the option reaches dumpbin correctly
        dumpbin //EXPORTS "$LIB_PATH" | awk '/^[[:space:]]*[0-9]+[[:space:]]+[0-9A-Fa-f]+[[:space:]]+[0-9A-Fa-f]+[[:space:]]+/ {print $4}' | grep -E "^WasmEdge" | sort > "$TEMP_DIR/extracted.symbols"
    elif command -v objdump >/dev/null 2>&1; then
        info "dumpbin not found, using objdump as fallback..."
        objdump -p "$LIB_PATH" | grep -E "^\s*\[[[:space:]]*[0-9]+\]" | grep "WasmEdge" | awk '{print $NF}' | sort > "$TEMP_DIR/extracted.symbols"
    else
        error "Neither dumpbin nor objdump found. Make sure Visual Studio tools are in PATH or MinGW is installed"
        exit 1
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS: Use nm to extract symbols (no -D flag on macOS)
    if ! command -v nm >/dev/null 2>&1; then
        error "nm not found. Make sure Xcode Command Line Tools are installed"
        exit 1
    fi
    
    debug "Attempting to extract symbols from macOS dylib..."
    debug "Library info: $(file "$LIB_PATH" 2>/dev/null || echo "file command not available")"
    
    # Try different nm approaches for macOS - NOTE: macOS symbols have underscore prefix
    if nm -g "$LIB_PATH" 2>/dev/null | awk '/^[0-9a-fA-F]+ [TBDWS] _WasmEdge/ {print substr($3,2)}' | sort > "$TEMP_DIR/extracted.symbols"; then
        debug "Successfully extracted symbols using nm -g (removed underscore prefix)"
    elif nm -D "$LIB_PATH" 2>/dev/null | awk '/^[0-9a-fA-F]+ [TBDWS] _WasmEdge/ {print substr($3,2)}' | sort > "$TEMP_DIR/extracted.symbols"; then
        debug "Successfully extracted symbols using nm -D (removed underscore prefix)"
    elif nm "$LIB_PATH" 2>/dev/null | awk '/^[0-9a-fA-F]+ [TBDWS] _WasmEdge/ {print substr($3,2)}' | sort > "$TEMP_DIR/extracted.symbols"; then
        debug "Successfully extracted symbols using nm (removed underscore prefix)"
    elif nm "$LIB_PATH" 2>/dev/null | grep "_WasmEdge" | awk '{print substr($NF,2)}' | sort > "$TEMP_DIR/extracted.symbols"; then
        debug "Successfully extracted symbols using simplified parsing (removed underscore prefix)"
    else
        error "Failed to extract WasmEdge symbols with nm."
        debug "Trying to show WasmEdge-related nm output for troubleshooting..."
        debug "nm -g output (WasmEdge symbols only, first 10):"
        if [ "$VERBOSE" = true ]; then
            nm -g "$LIB_PATH" 2>&1 | grep "_WasmEdge" | head -10 || debug "No WasmEdge symbols found with nm -g"
        fi
        exit 1
    fi
else
    # Linux: Use nm to extract symbols
    if ! command -v nm >/dev/null 2>&1; then
        error "nm not found. Install binutils package"
        exit 1
    fi
    nm -D --defined-only "$LIB_PATH" | awk '/^[0-9a-fA-F]+ [TBDW] / {print $3}' | grep -E "^WasmEdge" | sort > "$TEMP_DIR/extracted.symbols"
fi

# Verify that we extracted some symbols
if [ ! -s "$TEMP_DIR/extracted.symbols" ]; then
    error "No WasmEdge symbols found in library. This might indicate:"
    eprintf "  1. Library was not built correctly"
    eprintf "  2. Symbol extraction failed"
    eprintf "  3. All symbols are hidden (check build configuration)"
    eprintf "  4. Library is static or has no dynamic symbol table"
    eprintf ""
    debug "Debug information:"
    debug "Library file: $LIB_PATH"
    debug "Library size: $(ls -lh "$LIB_PATH" | awk '{print $5}')"
    debug "Library type: $(file "$LIB_PATH" 2>/dev/null || echo "unknown")"
    exit 1
fi

# Sort whitelist for comparison and convert line endings
sort "$WHITELIST_FILE" | tr -d '\r' > "$TEMP_DIR/whitelist_sorted.symbols"

# Note: extracted.symbols is already sorted during symbol extraction
# Find symbols in extracted but not in whitelist (unexpected)
grep -Fxv -f "$TEMP_DIR/whitelist_sorted.symbols" "$TEMP_DIR/extracted.symbols" > "$TEMP_DIR/unexpected.symbols" 2>/dev/null || true

# Find symbols in whitelist but not in extracted (missing)
grep -Fxv -f "$TEMP_DIR/extracted.symbols" "$TEMP_DIR/whitelist_sorted.symbols" > "$TEMP_DIR/missing.symbols" 2>/dev/null || true

# Report results
EXTRACTED_COUNT=$(wc -l < "$TEMP_DIR/extracted.symbols")
WHITELIST_COUNT=$(wc -l < "$TEMP_DIR/whitelist_sorted.symbols")
UNEXPECTED_COUNT=$(wc -l < "$TEMP_DIR/unexpected.symbols")
MISSING_COUNT=$(wc -l < "$TEMP_DIR/missing.symbols")

info "Symbol analysis results:"
debug "  Extracted symbols: $EXTRACTED_COUNT"
debug "  Whitelisted symbols: $WHITELIST_COUNT"
debug "  Unexpected symbols: $UNEXPECTED_COUNT"
debug "  Missing symbols: $MISSING_COUNT"

# Show unexpected symbols if any
if [ "$UNEXPECTED_COUNT" -gt 0 ]; then
    eprintf ""
    error "Found unexpected symbols exported by libwasmedge:"
    cat "$TEMP_DIR/unexpected.symbols"
    eprintf ""
    eprintf "These symbols should either be:"
    eprintf "1. Added to the whitelist if they are intended public API"
    eprintf "2. Made static or marked with hidden visibility if they are internal"
    eprintf ""
    exit 1
fi

# Show missing symbols if any (warning only)
if [ "$MISSING_COUNT" -gt 0 ]; then
    eprintf ""
    warn "Some whitelisted symbols were not found in the library:"
    cat "$TEMP_DIR/missing.symbols"
    eprintf ""
    eprintf "This might indicate:"
    eprintf "1. Symbols removed from the API (update whitelist)"
    eprintf "2. Conditional compilation excluding some symbols"
    eprintf "3. Platform-specific symbols not available on this build"
    eprintf ""
fi

info "✅ Symbol exposure check passed - no unexpected symbols found!"
exit 0
