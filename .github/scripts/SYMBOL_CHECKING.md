# WasmEdge Symbol Exposure Checking

Prevents unintended symbol exposure from `libwasmedge` by validating against a whitelist.

## Background

Issue #3743: WasmEdge 0.14.0 accidentally exposed hundreds of internal symbols due to CMake changes. This system prevents regressions.

## Components

- `check_symbols.sh` - Cross-platform symbol extraction and validation script
- `whitelist.symbols` - List of approved exported symbols

## Usage

```bash
bash .github/scripts/check_symbols.sh
```

The script:
1. Extracts symbols from built library (`nm`/`dumpbin`)
2. Compares against whitelist
3. Reports unexpected symbols (CI fails) or missing symbols (warning)

## Platform Support

- **Linux**: `nm -D --defined-only`
- **macOS**: `nm -g` (removes underscore prefix)
- **Windows**: `dumpbin //EXPORTS` (objdump fallback)
- **Static builds**: Auto-detected and skipped

## Updating Whitelist

Add new symbols alphabetically when adding public API functions:

```
feat(api): add new validation function

- Add WasmEdge_ValidatorCreateWithConfig to whitelist

Signed-off-by: Your Name <your.email@example.com>
```

Remove symbols when deprecating API functions:

```
feat(api): remove deprecated function

- Remove WasmEdge_OldFunction from whitelist

Signed-off-by: Your Name <your.email@example.com>
```

## Troubleshooting

- **Library not found**: Ensure `cmake --build build` completed
- **No symbols found**: Check if library is stripped
- **Tool errors**: Install platform tools (binutils, Xcode CLI, VS tools)
