#!/bin/bash

set -e

error_exit() {
    echo "Error: $1" >&2
    exit 1
}

check_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        error_exit "Required command '$1' not found. Please install it first."
    fi
}

case "$(uname -s)" in
    Linux*)     
        OS_TYPE=linux
        check_command nm
        check_command awk
        check_command sort
        ;;
    Darwin*)    
        OS_TYPE=macos
        check_command nm
        check_command awk
        check_command sort
        ;;
    MINGW*|MSYS*|CYGWIN*) 
        OS_TYPE=windows
        check_command dumpbin
        check_command awk
        check_command sort
        ;;
    *)          
        error_exit "Unsupported operating system"
        ;;
esac

TEMP_DIR=$(mktemp -d)
if [ ! -d "$TEMP_DIR" ]; then
    error_exit "Failed to create temporary directory"
fi

cleanup() {
    rm -rf "$TEMP_DIR"
}
trap cleanup EXIT INT TERM

if [ "$OS_TYPE" = "linux" ]; then
    LIB_PATH="${SYMBOLS_PATH:-build}/lib/api/libwasmedge.so"
elif [ "$OS_TYPE" = "macos" ]; then
    LIB_PATH="${SYMBOLS_PATH:-build}/lib/api/libwasmedge.dylib"
elif [ "$OS_TYPE" = "windows" ]; then
    LIB_PATH="${SYMBOLS_PATH:-build}/lib/api/wasmedge.dll"
fi

[ -f "$LIB_PATH" ] || error_exit "Library not found at $LIB_PATH"

# Path to whitelist file
WHITELIST="lib/api/whitelist.symbols"
[ -f "$WHITELIST" ] || error_exit "Whitelist file not found at $WHITELIST"

LOG_FILE="$TEMP_DIR/symbols_check.log"

echo "Checking symbols in $LIB_PATH..."
echo "Using whitelist from $WHITELIST"
echo "Detailed log will be written to $LOG_FILE"

{
    echo "=== Symbol Check Log ==="
    echo "Date: $(date)"
    echo "Library: $LIB_PATH"
    echo "Whitelist: $WHITELIST"
    echo "======================\n"
} > "$LOG_FILE"

if [ "$OS_TYPE" = "linux" ]; then
    nm -D --defined-only "$LIB_PATH" | awk '{print $3}' | grep -v '^$' | sort > "$TEMP_DIR/extracted.symbols" 2>> "$LOG_FILE"
elif [ "$OS_TYPE" = "macos" ]; then
    nm -g "$LIB_PATH" | awk '$2 ~ /^[TD]$/ {print $3}' | grep -v '^$' | sort > "$TEMP_DIR/extracted.symbols" 2>> "$LOG_FILE"
elif [ "$OS_TYPE" = "windows" ]; then
    dumpbin /EXPORTS "$LIB_PATH" | grep -A9999 "ordinal hint" | grep -B9999 "Summary" | tail -n +2 | head -n -2 | awk '{print $4}' | grep -v '^$' | sort > "$TEMP_DIR/extracted.symbols" 2>> "$LOG_FILE"
fi

sort "$WHITELIST" > "$TEMP_DIR/whitelist.sorted"

comm -23 "$TEMP_DIR/extracted.symbols" "$TEMP_DIR/whitelist.sorted" > "$TEMP_DIR/unexpected.symbols"

comm -13 "$TEMP_DIR/extracted.symbols" "$TEMP_DIR/whitelist.sorted" > "$TEMP_DIR/missing.symbols"


UNEXPECTED_COUNT=$(wc -l < "$TEMP_DIR/unexpected.symbols")
MISSING_COUNT=$(wc -l < "$TEMP_DIR/missing.symbols")

{
    echo "\n=== Results ==="
    echo "Total symbols in library: $(wc -l < "$TEMP_DIR/extracted.symbols")"
    echo "Total symbols in whitelist: $(wc -l < "$TEMP_DIR/whitelist.sorted")"
    
    if [ "$UNEXPECTED_COUNT" -gt 0 ]; then
        echo "\nUnexpected symbols found:"
        cat "$TEMP_DIR/unexpected.symbols"
    fi
    
    if [ "$MISSING_COUNT" -gt 0 ]; then
        echo "\nMissing symbols:"
        cat "$TEMP_DIR/missing.symbols"
    fi
} >> "$LOG_FILE"

if [ "$UNEXPECTED_COUNT" -gt 0 ]; then
    echo "Error: Found $UNEXPECTED_COUNT unexpected symbol(s). See $LOG_FILE for details."
    echo "First few unexpected symbols:"
    head -n 5 "$TEMP_DIR/unexpected.symbols"
    if [ "$UNEXPECTED_COUNT" -gt 5 ]; then
        echo "... and $(($UNEXPECTED_COUNT - 5)) more"
    fi
    exit 1
fi

if [ "$MISSING_COUNT" -gt 0 ]; then
    echo "Warning: $MISSING_COUNT symbol(s) from whitelist not found in library. See $LOG_FILE for details."
fi

echo "Symbol check passed successfully!"
echo "See $LOG_FILE for detailed information."
exit 0 