# WasmEdge Symbol Exposure Checking

Prevents unintended symbol exposure from `libwasmedge` by validating against a whitelist.

## Background

[Issue #3655](https://github.com/WasmEdge/WasmEdge/issues/3655): WasmEdge 0.14.0 accidentally exposed hundreds of internal C++ symbols due to a broad pattern in the linker script. This CI check prevents regressions. This adds two new components.

- `check_symbols.sh` - simple script for symbol extraction and validation
- `whitelist.symbols` - Approved exported symbols list

## Scope

The check covers three exported namespaces:

- `WasmEdge_*` - WasmEdge C API, declared with `WASMEDGE_CAPI_EXPORT`.
- `wasm_*` - wasm-c-api C functions (`thirdparty/wasm/wasm.h`).
- `wasm::*` - wasm-c-api C++ classes (`thirdparty/wasm/wasm.hh`); demangled and normalized so the whitelist is toolchain-agnostic (libc++ `std::__1::` and libstdc++ `std::__cxx11::` inline namespaces are stripped to plain `std::`).

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

1. Add the function to the appropriate public header:
   - `include/api/wasmedge/wasmedge.h` with `WASMEDGE_CAPI_EXPORT` for the `WasmEdge_*` namespace.
   - `thirdparty/wasm/wasm.h` / `wasm.hh` for the wasm-c-api `wasm_*` / `wasm::*` namespaces.
2. Add the symbol name to `whitelist.symbols` (keep alphabetical order). For `wasm::*` methods, use the demangled, normalized form (e.g. `wasm::Memory::data() const`) as produced by `check_symbols.sh --verbose`.
3. Commit both changes together
