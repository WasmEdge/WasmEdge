# SPDX-License-Identifier: Apache-2.0
enable_language(ASM-ATT)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(SIMD_FILES
    blake3_avx2.c
    blake3_sse2.c
    blake3_sse41.c
    blake3_avx2_x86-64_unix.S
    blake3_sse2_x86-64_unix.S
    blake3_sse41_x86-64_unix.S
  )
  set_source_files_properties(blake3_avx2.c PROPERTIES COMPILE_FLAGS -mavx2)
  set_source_files_properties(blake3_sse2.c PROPERTIES COMPILE_FLAGS -msse2)
  set_source_files_properties(blake3_sse41.c PROPERTIES COMPILE_FLAGS -msse4.1)
  set(SIMD_DEFS)

  if (CMAKE_ASM-ATT_COMPILER_ID STREQUAL "GNU")
    execute_process(
      COMMAND ${CMAKE_ASM-ATT_COMPILER} --help
      OUTPUT_VARIABLE _gas_help
    )
    if(_gas_help MATCHES "avx512vl")
      set(SIMD_FILES ${SIMD_FILES} blake3_avx512.c blake3_avx512_x86-64_unix.S)
      set_source_files_properties(blake3_avx512.c PROPERTIES COMPILE_FLAGS "-mavx512f -mavx512vl")
    else()
      set(SIMD_DEFS ${SIMD_DEFS} -DBLAKE3_NO_AVX512)
    endif()
  endif()
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
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

add_library(utilBlake3
  blake3.c
  blake3_dispatch.c
  blake3_portable.c
  ${SIMD_FILES}
)

target_include_directories(utilBlake3
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(utilBlake3
  PRIVATE
  ${SIMD_DEFS}
)
