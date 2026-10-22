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

## C API symbol versioning

On Linux/Android, `lib/api/libwasmedge.lds` groups the exported C symbols into
version nodes (`WASMEDGE_0.16`, `WASMEDGE_0.17`, ...), so one
`libwasmedge.so.0` serves both the old and the new ABI of a changed function
without a SONAME bump. macOS and Windows export the symbols unversioned.

When a release changes the signature of a C API function (parameters, return
type, or a by-value struct layout):

1. Add a node named after the release to `libwasmedge.lds`, inheriting the
   previous node, and list the function under its `global:`. The current
   implementation becomes the default `<name>@@<node>`.
2. Add a shim `WasmEdge_<Name>_Compat_<old>` with the old implementation to
   `lib/api/wasmedge_compat.cpp` and bind it with
   `__asm__(".symver WasmEdge_<Name>_Compat_<old>, WasmEdge_<Name>@<prev>")`.
   `<prev>` is the node the symbol had in the previous release, taken from
   `readelf --dyn-syms --wide` of the released library: a function that never
   changed before is in the base node `WASMEDGE_0.16`.
3. Mark the shim `__attribute__((used, visibility("default")))`, since a hidden
   symbol exports no alias, and add its name to the `local:` section of the base
   node so that only the alias is exported.

The whitelist needs no change, because `check_symbols.sh` strips the
`@<version>` suffix. `wasmedge_compat.cpp` is a separate translation unit built
with `-fno-lto`: its `.symver` asm collides with the in-IR definition under
ThinLTO, and GCC LTO drops the shims together with their aliases.

Examples:

- 0.16 -> 0.17: the five `WasmEdge_Limit` functions export `@@WASMEDGE_0.17`
  and `@WASMEDGE_0.16`.
- 0.17 -> 0.18: `WasmEdge_ModuleInstanceAdd{Function,Table,Memory,Global}`
  export `@@WASMEDGE_0.18` and `@WASMEDGE_0.16`, because 0.17.x exported them
  from the base node.

A consumer built against an unversioned library (0.16.x or 0.17.0) binds to the
oldest version, `@WASMEDGE_0.16`, not to the `@@` default. 0.16.x consumers keep
working, while 0.17.0 consumers of the limit functions must be rebuilt.

## Symbol version check

On Linux, `check_symbols.sh` also requires `<name>@@<node>` and
`<name>@WASMEDGE_0.16` for every function listed in a non-base node of
`libwasmedge.lds`. Without the alias, older consumers fail to load with
`undefined symbol: <name>, version WASMEDGE_0.16`. The CI runs the check on the
Ubuntu (Clang ThinLTO) and manylinux (GCC LTO) builds. A second signature change
of the same function, which needs an alias at its previous non-base node, is not
checked.
