# WasmEdge Symbol Exposure Checking

Prevents unintended symbol exposure from `libwasmedge` by validating against a whitelist.

## Background

[Issue #3655](https://github.com/WasmEdge/WasmEdge/issues/3655): WasmEdge 0.14.0 accidentally exposed hundreds of internal C++ symbols due to a broad pattern in the linker script. This CI check prevents regressions. This adds two new components.

- `check_symbols.sh` - simple script for symbol extraction and validation
- `whitelist.symbols` - Approved exported symbols list

## Usage

Run from project root after building

```bash
bash .github/scripts/check_symbols.sh
```

Verbose mode for debugging

```bash
bash .github/scripts/check_symbols.sh --verbose
```

Override library path

```bash
LIB_PATH=/path/to/libwasmedge.so bash .github/scripts/check_symbols.sh
```

Static-only builds are detected and skipped.

## Updating Whitelist

When adding new public API functions:

1. Add the function to `include/api/wasmedge/wasmedge.h` with `WASMEDGE_CAPI_EXPORT`
2. Add the symbol name to `whitelist.symbols` (keep alphabetical order)
3. Commit both changes together
