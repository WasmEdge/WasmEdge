# RISC-V Cross-Compilation for WasmEdge

This directory contains the toolchain and scripts needed for cross-compiling WasmEdge for RISC-V 64-bit architecture.

## Overview

The RISC-V build process has been optimized to use cross-compilation instead of full emulation, significantly reducing CI build times from ~40 minutes to ~10-15 minutes.

## Files

### `cmake/riscv64-linux-gnu.cmake`
CMake toolchain file for RISC-V cross-compilation. This file:
- Sets up the cross-compilation environment
- Configures RISC-V GCC/G++ compilers
- Sets appropriate compiler and linker flags for RV64GC architecture
- Includes error checking for missing toolchain components

### `utils/setup-riscv-cross.sh`
Setup script that installs the RISC-V cross-compilation toolchain and dependencies:
- Installs `gcc-riscv64-linux-gnu` and related packages
- Verifies toolchain installation
- Installs additional build dependencies (CMake, LLVM, etc.)
- Includes comprehensive error checking and logging

### `utils/test-riscv-qemu.sh`
Test script for validating cross-compiled binaries in QEMU environment:
- Sets up the runtime environment for testing
- Runs comprehensive functionality tests
- Validates AOT compilation and execution
- Includes timeout protection and detailed logging

## Usage

### Local Development

1. **Install the toolchain:**
   ```bash
   chmod +x utils/setup-riscv-cross.sh
   ./utils/setup-riscv-cross.sh
   ```

2. **Cross-compile WasmEdge:**
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_TOOLCHAIN_FILE=../cmake/riscv64-linux-gnu.cmake \
         -DWASMEDGE_BUILD_TOOLS=ON \
         -DWASMEDGE_BUILD_SHARED_LIB=ON \
         -DWASMEDGE_USE_LLVM=OFF \
         ..
   make -j$(nproc)
   ```

3. **Test in QEMU (if available):**
   ```bash
   # Package artifacts for testing
   mkdir riscv64-artifacts
   cp -r build/lib build/tools examples riscv64-artifacts/
   
   # Run tests in QEMU environment
   ./utils/test-riscv-qemu.sh
   ```

### CI Integration

The CI workflow (`.github/workflows/build_for_riscv.yml`) automatically:
1. Sets up the cross-compilation environment
2. Cross-compiles WasmEdge for RISC-V
3. Packages the build artifacts
4. Tests the binaries in a QEMU RISC-V environment

## Configuration Options

### Environment Variables

The test script supports these environment variables:

- `WASMEDGE_ROOT`: Root directory of the WasmEdge project (default: `/workplace`)
- `ARTIFACTS_DIR`: Directory containing build artifacts (default: `${WASMEDGE_ROOT}/riscv64-artifacts`)
- `TIMEOUT_DURATION`: Timeout for individual tests in seconds (default: `60`)

### CMake Options

When cross-compiling, these options are recommended:

- `DWASMEDGE_USE_LLVM=OFF`: Disable LLVM to avoid cross-compilation complexity
- `DWASMEDGE_BUILD_PLUGINS=OFF`: Disable plugins for initial implementation
- `DWASMEDGE_BUILD_TOOLS=ON`: Build essential tools
- `DWASMEDGE_BUILD_SHARED_LIB=ON`: Build shared libraries

## Troubleshooting

### Common Issues

1. **Toolchain not found:**
   - Ensure `gcc-riscv64-linux-gnu` is installed
   - Check that the compiler is in your PATH
   - Verify installation with: `riscv64-linux-gnu-gcc --version`

2. **CMake configuration fails:**
   - Ensure CMake 3.18+ is installed
   - Check that the toolchain file path is correct
   - Verify all dependencies are installed

3. **Tests fail in QEMU:**
   - Check that artifacts were packaged correctly
   - Ensure binaries are executable
   - Verify library dependencies are met

### Performance Notes

- Cross-compilation builds much faster than emulation (~5-10x speedup)
- QEMU testing adds minimal overhead compared to full emulated builds
- The approach scales well for larger codebases

## Architecture

The RISC-V cross-compilation setup targets:
- **Architecture:** RV64GC (64-bit RISC-V with standard extensions)
- **ABI:** LP64D (64-bit pointers, 64-bit long, double-precision floating point)
- **Target Triple:** riscv64-linux-gnu

## Contributing

When modifying the RISC-V build system:

1. Test changes locally first
2. Ensure all scripts remain POSIX-compliant where possible
3. Add appropriate error handling and logging
4. Update this documentation for any configuration changes
5. Verify CI builds complete successfully
