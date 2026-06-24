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

## C API symbol versioning

On Linux/Android the exported C symbols are grouped into versioned nodes by the
linker version script `lib/api/libwasmedge.lds` (e.g. `WASMEDGE_0.16`,
`WASMEDGE_0.17`). This lets a single `libwasmedge.so.0` keep serving an older
ABI alongside the current one, so a signature-only break does not force every
dynamically linked consumer to rebuild and does not, by itself, require a
SONAME (`WASMEDGE_CAPI_SOVERSION`) bump.

When a release breaks the signature of an existing C API function (changed
parameters/return type or a changed by-value struct layout):

1. Add a new version node to `lib/api/libwasmedge.lds` named after the release
   that introduces the break (inheriting the previous node), and list the
   changed symbols under its `global:` section. The literal names take
   precedence over the `WasmEdge*` wildcard, so the current implementations
   become the new node's default (`@@`) symbols automatically.
2. Keep the previous behaviour available by adding a backward-compatible shim in
   `lib/api/wasmedge_compat.cpp` (guarded by `#if defined(__ELF__) &&
   (defined(__linux__) || defined(__ANDROID__))`) named
   `WasmEdge_<Name>_Compat_<old>`, and bind it to the old version with
   `__asm__(".symver WasmEdge_<Name>_Compat_<old>, WasmEdge_<Name>@WASMEDGE_<old>")`.
   The shim lives in its own translation unit (compiled only into the shared
   library) so that this `.symver` module-level asm does not collide with the
   in-IR definition of `WasmEdge_<Name>` under ThinLTO.
   Mark the shim `__attribute__((used, visibility("default")))`. The
   `visibility("default")` is required: the project builds with
   `-fvisibility=hidden`, and a hidden source symbol does not produce an exported
   `.symver` alias (the compat symbol silently disappears). Because that default
   visibility plus the `WasmEdge_*` name would otherwise export the shim's own
   name, add its exact name to the `local:` section of the base node in
   `lib/api/libwasmedge.lds` (an exact match outranks the `WasmEdge*` wildcard),
   so only the `@WASMEDGE_<old>` alias is exported.
3. `check_symbols.sh` strips the `@<version>` suffix before matching, so the
   `whitelist.symbols` entries stay the bare function names — no whitelist
   changes are needed for versioning.

The limit struct -> limit context change (0.16 -> 0.17) is the worked example:
`WasmEdge_LimitIsEqual`, `WasmEdge_MemoryTypeCreate`, `WasmEdge_MemoryTypeGetLimit`,
`WasmEdge_TableTypeCreate`, and `WasmEdge_TableTypeGetLimit` carry both a
`@@WASMEDGE_0.17` default and a `@WASMEDGE_0.16` compatibility symbol.

> Note: this mechanism is ELF-only; macOS and Windows export the symbols
> unversioned. A subtle but important runtime rule: an *unversioned* reference
> (from a consumer built against a pre-versioning library — i.e. 0.16.x or the
> released 0.17.0) binds to the **oldest** matching version definition, here
> `@WASMEDGE_0.16` (the struct shim), **not** to the default `@@` symbol. The
> `@@` default only selects what *newly linked* code records (it gets a
> versioned `@WASMEDGE_0.17` reference). Consequences:
>
> - Consumers built against **0.16.x keep working without a rebuild** — their
>   unversioned references land on the matching `@WASMEDGE_0.16` struct shim.
> - Consumers built against the (also unversioned) **0.17.0 must be rebuilt** —
>   their unversioned references also land on the `@WASMEDGE_0.16` struct shim,
>   which mismatches the pointer/context ABI they were compiled for.
> - Consumers built against this versioned library resolve to `@WASMEDGE_0.17`
>   and are correct.
