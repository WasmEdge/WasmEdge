# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2024 WasmEdge Contributors

# Cross-compilation toolchain for RISC-V 64-bit
cmake_minimum_required(VERSION 3.18)

# Set the target system
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

# Specify the cross compiler - check multiple possible locations
find_program(RISCV64_GCC 
    NAMES riscv64-linux-gnu-gcc
    PATHS /usr/bin /usr/local/bin
    NO_DEFAULT_PATH
)
find_program(RISCV64_GXX 
    NAMES riscv64-linux-gnu-g++
    PATHS /usr/bin /usr/local/bin  
    NO_DEFAULT_PATH
)

if(NOT RISCV64_GCC OR NOT RISCV64_GXX)
    message(FATAL_ERROR "RISC-V cross-compilation toolchain not found. Please install gcc-riscv64-linux-gnu package.")
endif()

set(CMAKE_C_COMPILER ${RISCV64_GCC})
set(CMAKE_CXX_COMPILER ${RISCV64_GXX})

# Specify the target environment
set(CMAKE_FIND_ROOT_PATH /usr/riscv64-linux-gnu)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# For libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Set RISC-V specific compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=rv64gc -mabi=lp64d")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=rv64gc -mabi=lp64d")

# Set linker flags for better compatibility
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")

# Additional flags for WasmEdge specific needs
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

# Set pkg-config for cross-compilation if available
find_program(RISCV64_PKG_CONFIG 
    NAMES riscv64-linux-gnu-pkg-config
    PATHS /usr/bin /usr/local/bin
    NO_DEFAULT_PATH
)
if(RISCV64_PKG_CONFIG)
    set(PKG_CONFIG_EXECUTABLE ${RISCV64_PKG_CONFIG})
endif()
