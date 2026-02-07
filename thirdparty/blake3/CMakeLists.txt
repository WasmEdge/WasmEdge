# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

enable_language(ASM)

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  set(BLAKE3_CFLAGS_SSE2 "/arch:SSE2" CACHE STRING "the compiler flags to enable SSE2")
  # MSVC has no dedicated sse4.1 flag (see https://learn.microsoft.com/en-us/cpp/build/reference/arch-x86?view=msvc-170)
  set(BLAKE3_CFLAGS_SSE4.1 "/arch:AVX" CACHE STRING "the compiler flags to enable SSE4.1")
  set(BLAKE3_CFLAGS_AVX2 "/arch:AVX2" CACHE STRING "the compiler flags to enable AVX2")
  set(BLAKE3_CFLAGS_AVX512 "/arch:AVX512" CACHE STRING "the compiler flags to enable AVX512")
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU"
       OR CMAKE_C_COMPILER_ID STREQUAL "Clang"
       OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
  set(BLAKE3_CFLAGS_SSE2 "-msse2" CACHE STRING "the compiler flags to enable SSE2")
  set(BLAKE3_CFLAGS_SSE4.1 "-msse4.1" CACHE STRING "the compiler flags to enable SSE4.1")
  set(BLAKE3_CFLAGS_AVX2 "-mavx2" CACHE STRING "the compiler flags to enable AVX2")
  set(BLAKE3_CFLAGS_AVX512 "-mavx512f -mavx512vl" CACHE STRING "the compiler flags to enable AVX512")
endif()
# architecture lists for which to enable assembly / SIMD sources
set(BLAKE3_AMD64_NAMES amd64 AMD64 x86_64)
set(BLAKE3_X86_NAMES i686 x86 X86)
set(BLAKE3_ARMv8_NAMES aarch64 AArch64 arm64 ARM64 armv8 armv8a)

wasmedge_add_library(utilBlake3
  blake3.c
  blake3_dispatch.c
  blake3_portable.c
)

# optional SIMD sources
macro(BLAKE3_DISABLE_SIMD)
  set(BLAKE3_SIMD_AMD64_ASM OFF)
  set(BLAKE3_SIMD_X86_INTRINSICS OFF)
  set(BLAKE3_SIMD_NEON_INTRINSICS OFF)
  set_source_files_properties(blake3_dispatch.c PROPERTIES
    COMPILE_DEFINITIONS "BLAKE3_USE_NEON=0;BLAKE3_NO_SSE2;BLAKE3_NO_SSE41;BLAKE3_NO_AVX2;BLAKE3_NO_AVX512"
  )
endmacro()

BLAKE3_DISABLE_SIMD()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(utilBlake3
    PRIVATE
    -Wno-assign-enum
    -Wno-cast-align
    -Wno-cast-qual
    -Wno-implicit-int-conversion
    -Wno-implicit-int-enum-cast
    -Wno-language-extension-token
    -Wno-missing-prototypes
    -Wno-pointer-sign
    -Wno-shorten-64-to-32
    -Wno-sign-conversion
    -Wno-unused-function
  )
endif()

target_include_directories(utilBlake3
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
