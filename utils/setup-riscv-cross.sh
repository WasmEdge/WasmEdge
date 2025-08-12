#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2024 WasmEdge Contributors

# Script to set up RISC-V cross-compilation environment for WasmEdge CI
# This script installs the necessary toolchain and dependencies for cross-compiling
# WasmEdge for RISC-V 64-bit architecture.

set -euo pipefail

# Function to log messages
log() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $*"
}

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to verify package installation
verify_package() {
    if dpkg -l "$1" >/dev/null 2>&1; then
        log "✓ Package $1 is installed"
        return 0
    else
        log "✗ Package $1 is not installed"
        return 1
    fi
}

log "Setting up RISC-V cross-compilation environment..."

# Update package lists
log "Updating package lists..."
sudo apt-get update -q -y

# Install RISC-V toolchain
log "Installing RISC-V cross-compilation toolchain..."
sudo apt-get install -q -y \
    gcc-riscv64-linux-gnu \
    g++-riscv64-linux-gnu \
    libc6-dev-riscv64-cross

# Verify toolchain installation
log "Verifying RISC-V toolchain installation..."
if command_exists riscv64-linux-gnu-gcc; then
    log "✓ RISC-V GCC version: $(riscv64-linux-gnu-gcc --version | head -n1)"
else
    log "✗ RISC-V GCC not found in PATH"
    exit 1
fi

if command_exists riscv64-linux-gnu-g++; then
    log "✓ RISC-V G++ version: $(riscv64-linux-gnu-g++ --version | head -n1)"
else
    log "✗ RISC-V G++ not found in PATH"
    exit 1
fi

# Install other required dependencies
log "Installing additional build dependencies..."
sudo apt-get install -q -y \
    git \
    cmake \
    dpkg \
    software-properties-common \
    llvm-15-dev \
    liblld-15-dev \
    zlib1g-dev \
    wabt

# Verify critical packages
critical_packages=("gcc-riscv64-linux-gnu" "g++-riscv64-linux-gnu" "cmake" "wabt")
for package in "${critical_packages[@]}"; do
    if ! verify_package "$package"; then
        log "Critical package $package verification failed"
        exit 1
    fi
done

# Test basic cross-compilation
log "Testing basic cross-compilation..."
cat > /tmp/test_riscv.c << 'EOF'
#include <stdio.h>
int main() {
    printf("Hello RISC-V!\n");
    return 0;
}
EOF

if riscv64-linux-gnu-gcc -o /tmp/test_riscv /tmp/test_riscv.c; then
    log "✓ Basic cross-compilation test passed"
    rm -f /tmp/test_riscv /tmp/test_riscv.c
else
    log "✗ Basic cross-compilation test failed"
    exit 1
fi

log "🎉 RISC-V cross-compilation environment setup completed successfully!"
log "Toolchain location: $(which riscv64-linux-gnu-gcc)"
log "Target triple: riscv64-linux-gnu"
