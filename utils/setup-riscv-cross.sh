#!/bin/bash
# Script to set up RISC-V cross-compilation environment for WasmEdge CI

set -e

echo "Setting up RISC-V cross-compilation environment..."

# Install RISC-V toolchain
sudo apt-get update -q -y
sudo apt-get install -q -y \
    gcc-riscv64-linux-gnu \
    g++-riscv64-linux-gnu \
    libc6-dev-riscv64-cross

# Verify toolchain installation
echo "Verifying RISC-V toolchain..."
riscv64-linux-gnu-gcc --version
riscv64-linux-gnu-g++ --version

# Install other required dependencies
sudo apt-get install -q -y \
    git cmake dpkg \
    software-properties-common \
    llvm-15-dev liblld-15-dev \
    zlib1g-dev \
    wabt

echo "RISC-V cross-compilation environment setup complete!"
