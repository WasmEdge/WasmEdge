# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

enable_language(ASM-ATT OPTIONAL)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
  set(SIMD_FILES
    blake3_avx2.c
    blake3_sse2.c
    blake3_sse41.c
  )
  set(SIMD_AVX512_FILES
    blake3_avx512.c
  )
  set(SIMD_UNIX_FILES
    blake3_avx2_x86-64_unix.S
    blake3_sse2_x86-64_unix.S
    blake3_sse41_x86-64_unix.S
  )
  set(SIMD_AVX512_UNIX_FILES
    blake3_avx512_x86-64_unix.S
  )
  set(SIMD_WIN_FILES
    blake3_avx2_x86-64_windows_gnu.S
    blake3_sse2_x86-64_windows_gnu.S
    blake3_sse41_x86-64_windows_gnu.S
  )
  set(SIMD_AVX512_WIN_FILES
    blake3_avx512_x86-64_unix.S
  )
  set_source_files_properties(blake3_avx2.c PROPERTIES COMPILE_FLAGS -mavx2)
  set_source_files_properties(blake3_sse2.c PROPERTIES COMPILE_FLAGS -msse2)
  set_source_files_properties(blake3_sse41.c PROPERTIES COMPILE_FLAGS -msse4.1)
  set(SIMD_DEFS)

  if(CMAKE_ASM-ATT_COMPILER_ID STREQUAL "GNU")
    execute_process(
      COMMAND ${CMAKE_ASM-ATT_COMPILER} --help
      OUTPUT_VARIABLE _gas_help
    )
    if(_gas_help MATCHES "avx512vl" OR _gas_help MATCHES "llvm")
      set(BLAKE3_AVX512 ON)
    else()
      set(BLAKE3_AVX512 OFF)
    endif()
  elseif(CMAKE_ASM-ATT_COMPILER_ID STREQUAL "Clang")
    set(BLAKE3_AVX512 ON)
  elseif(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(BLAKE3_AVX512 ON)
  endif()

  if(WIN32)
    set(SIMD_FILES ${SIMD_FILES} ${SIMD_WIN_FILES})
  else()
    set(SIMD_FILES ${SIMD_FILES} ${SIMD_UNIX_FILES})
  endif()

  if(BLAKE3_AVX512)
    set(SIMD_FILES ${SIMD_FILES} ${SIMD_AVX512_FILES})
    set_source_files_properties(blake3_avx512.c PROPERTIES COMPILE_FLAGS "-mavx512f -mavx512vl")
    if(WIN32)
      set(SIMD_FILES ${SIMD_FILES} ${SIMD_AVX512_WIN_FILES})
    else()
      set(SIMD_FILES ${SIMD_FILES} ${SIMD_AVX512_UNIX_FILES})
    endif()
  else()
    set(SIMD_DEFS ${SIMD_DEFS} -DBLAKE3_NO_AVX512)
  endif()

elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(aarch64|arm64)")
  set(SIMD_FILES
    blake3_neon.c
  )
  set(SIMD_DEFS
    -DBLAKE3_USE_NEON
  )
else()
  set(SIMD_FILES)
  set(SIMD_DEFS)
endif()

wasmedge_add_library(utilBlake3
  blake3.c
  blake3_dispatch.c
  blake3_portable.c
  ${SIMD_FILES}
)

target_compile_options(utilBlake3
  PRIVATE
  -Wno-assign-enum
  -Wno-cast-align
  -Wno-cast-qual
  -Wno-implicit-int-conversion
  -Wno-language-extension-token
  -Wno-missing-prototypes
  -Wno-pointer-sign
  -Wno-shorten-64-to-32
  -Wno-sign-conversion
  -Wno-unused-function
)

target_include_directories(utilBlake3
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(utilBlake3
  PRIVATE
  ${SIMD_DEFS}
)
