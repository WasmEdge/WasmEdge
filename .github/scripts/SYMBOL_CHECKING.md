# WasmEdge Symbol Exposure Management

## Overview

WasmEdge implements a strict symbol exposure policy to ensure that only intended public API symbols are exported from the `libwasmedge` shared library. This system prevents unintended symbol exposure that could affect ABI stability and security.

## Background

In version 0.14.0, WasmEdge experienced a regression where hundreds of internal C++ symbols were accidentally exposed due to a change in CMake configuration. This was fixed in 0.14.1, but to prevent future regressions, an automated checking system was implemented (GitHub issue #3743).

## Components

### 1. Symbol Whitelist (`whitelist.symbols`)

This file contains the complete list of symbols that are allowed to be exported from `libwasmedge`. It includes:

- All WasmEdge C API functions (e.g., `WasmEdge_VersionGet`, `WasmEdge_VMCreate`)
- Plugin creation functions
- Driver utility functions
- Essential utility functions

Currently maintains **282 approved symbols**.

### 2. Symbol Checking Script (`check_symbols.sh`)

An automated script that:

- Extracts symbols from the built library using platform-specific tools (`nm`, `dumpbin`, `objdump`)
- Compares exported symbols against the whitelist
- Reports any unexpected symbols
- Fails the build if unexpected symbols are found
- Gracefully handles static-only builds

### 3. CI Integration

The symbol checking is integrated into all major platform CI workflows:

- Ubuntu/Linux builds (with static build detection)
- macOS builds  
- Windows builds

The check runs after each successful build and before packaging.

## Usage

### Manual Symbol Check

To manually check symbol exposure:

```bash
# From the WasmEdge root directory (works on all platforms)
bash .github/scripts/check_symbols.sh
```

### Expected Output

**Success (no unexpected symbols):**
```
✅ Symbol exposure check passed - no unexpected symbols found!
  Extracted symbols: 282
  Whitelisted symbols: 282
  Unexpected symbols: 0
  Missing symbols: 0
```

**Failure (unexpected symbols found):**
```
❌ ERROR: Found unexpected symbols exported by libwasmedge:
SomeInternalFunction
AnotherPrivateSymbol

These symbols should either be:
1. Added to the whitelist if they are intended public API
2. Made static or marked with hidden visibility if they are internal
```

## Platform-Specific Behavior

**Linux/Ubuntu:** Uses `nm -D --defined-only` to extract dynamic symbols
**macOS:** Uses `nm -g` with fallbacks for different library formats  
**Windows:** Uses `dumpbin //EXPORTS` with `objdump` fallback
**Static Builds:** Automatically detected and skipped with success

## Whitelist Management Guidelines

### When to Update the Whitelist

**✅ Add symbols when:**
- Adding new public API functions
- Exposing previously internal functions that become public
- Adding new plugin interfaces

**❌ Don't add symbols for:**
- Internal implementation details
- Template instantiations
- C++ class internals
- Debug/development-only functions

### Updating Process

1. **Test first**: Run `bash .github/scripts/check_symbols.sh` before committing
2. **Add symbols alphabetically** in `whitelist.symbols`
3. **Document changes** in commit message using the template below
4. **Verify CI passes** on all platforms

### Commit Message Template

```
[API] Update symbol whitelist: [brief description]

- Added: WasmEdge_NewFunction1, WasmEdge_NewFunction2
- Removed: WasmEdge_OldFunction1 (deprecated)
- Reason: [explain why these changes are necessary]

Verified with: bash .github/scripts/check_symbols.sh

Signed-off-by: Your Name <your.email@example.com>
```

### Common Scenarios

**Adding New API Functions:**
1. Add function declaration to `include/api/wasmedge/wasmedge.h`
2. Implement the function in appropriate source file
3. Add function name to `whitelist.symbols` (alphabetically)
4. Run symbol check to verify
5. Commit with proper message template

**Removing Deprecated Functions:**
1. Remove function from `whitelist.symbols`
2. Update documentation about deprecation
3. Run symbol check to verify
4. Consider transition period for major changes

**Renaming Functions:**
1. Add new function name to whitelist
2. Keep old name temporarily for transition
3. Update all references in codebase
4. Remove old name after deprecation period

## Troubleshooting

**"Library not found" error:**
- Ensure WasmEdge is built: `cmake --build build`
- Check if it's a static-only build (will be auto-detected)

**"No symbols found" error:**
- Library might be stripped or static
- Check build configuration for symbol visibility

**"Tool not found" errors:**
- Linux: Install `binutils` package
- macOS: Install Xcode Command Line Tools
- Windows: Run in Visual Studio Developer Command Prompt

## Technical Details

The symbol checking system works by:

1. **Extracting symbols** from built libraries using platform tools
2. **Filtering** to only WasmEdge-prefixed symbols (`^WasmEdge`)
3. **Comparing** against the approved whitelist
4. **Reporting** any discrepancies with actionable guidance

This ensures a stable public API surface while preventing accidental exposure of internal implementation details.

## Files

- `check_symbols.sh` - Main checking script
- `whitelist.symbols` - Approved symbol list (282 symbols)  
- `SYMBOL_CHECKING.md` - This documentation file

All symbol management components are co-located in `.github/scripts/` for easy maintenance and Windows compatibility.
