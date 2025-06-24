# Symbol Exposure Management

## Overview

WasmEdge implements a strict symbol exposure policy to ensure that only intended public API symbols are exported from the `libwasmedge` shared library. This document describes the system in place to prevent unintended symbol exposure.

## Background

In version 0.14.0, WasmEdge experienced a regression where hundreds of internal C++ symbols were accidentally exposed due to a change in CMake configuration. This was fixed in 0.14.1, but to prevent future regressions, an automated checking system was implemented.

## Components

### 1. Symbol Whitelist (`lib/api/whitelist.symbols`)

This file contains the complete list of symbols that are allowed to be exported from `libwasmedge`. It includes:

- All WasmEdge C API functions (e.g., `WasmEdge_VersionGet`, `WasmEdge_VMCreate`)
- Plugin creation functions
- Essential utility functions

### 2. Symbol Checking Script (`.github/scripts/check_symbols.sh`)

An automated script that:

- Extracts symbols from the built library using platform-specific tools (`nm`, `dumpbin`)
- Compares exported symbols against the whitelist
- Reports any unexpected symbols
- Fails the build if unexpected symbols are found

### 3. CI Integration

The symbol checking is integrated into all major platform CI workflows:

- Ubuntu/Linux builds
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

Note: The script doesn't require execute permissions since it's invoked with `bash` directly.

### VS Code Task

A VS Code task is available:
- Open Command Palette (`Ctrl+Shift+P`)
- Run "Tasks: Run Task"
- Select "Check Symbol Exposure"

## Maintenance

### Adding New Public API Functions

When adding new public API functions:

1. Add the function to `include/api/wasmedge/wasmedge.h`
2. Implement the function in the appropriate source file
3. Add the function name to `lib/api/whitelist.symbols`
4. Ensure the function is properly exported in `lib/api/libwasmedge.lds`

### Handling Symbol Check Failures

If the CI fails with unexpected symbols:

1. **Review the symbols**: Check if they should be public API
2. **If they should be public**: Add them to the whitelist
3. **If they should be private**: 
   - Mark functions as `static` if they're file-local
   - Use anonymous namespaces for C++ internal functions
   - Apply `__attribute__((visibility("hidden")))` for specific symbols

### Troubleshooting

#### "Library not found" Error
Ensure the library is built before running the check:
```bash
cmake -Bbuild -GNinja
cmake --build build
```

#### Platform-Specific Issues

- **Linux**: Requires `nm` tool (usually available by default)
- **macOS**: Requires `nm` tool (from Xcode Command Line Tools)
- **Windows**: Requires `dumpbin` tool (from Visual Studio)

## Technical Details

### Symbol Visibility Control

WasmEdge uses several mechanisms to control symbol visibility:

1. **CMake Settings**: `CXX_VISIBILITY_PRESET hidden` in `cmake/Helper.cmake`
2. **Linker Script**: `lib/api/libwasmedge.lds` defines export patterns
3. **Compiler Attributes**: Explicit visibility control in source code

### Cross-Platform Considerations

The script handles platform differences:

- **Linux**: Uses `nm -D --defined-only` to extract dynamic symbols
- **macOS**: Uses `nm -D` with appropriate filtering
- **Windows**: Uses `dumpbin //EXPORTS` for DLL export analysis

## History

- **Issue #3655**: Original regression discovered in v0.14.0
- **PR #3656**: Quick fix by restoring symbol visibility settings
- **Issue #3743**: Design of systematic solution
- **PR #4001**: Initial implementation (incomplete)
- **Current**: Complete implementation with cross-platform support

## References

- [GitHub Issue #3655](https://github.com/WasmEdge/WasmEdge/issues/3655) - Original regression report
- [GitHub Issue #3743](https://github.com/WasmEdge/WasmEdge/issues/3743) - Prevention system design
- [CMake Documentation](https://cmake.org/cmake/help/latest/prop_tgt/CXX_VISIBILITY_PRESET.html) - Symbol visibility
