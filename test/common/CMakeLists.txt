# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeCommonTests
  int128Test.cpp
  hashTest.cpp
  spdlogTest.cpp
  roundevenTest.cpp
  spareEnumMapTest.cpp
  byteswapTest.cpp
  statisticsTest.cpp
  denseEnumMapTest.cpp
  bitTest.cpp
  simdTest.cpp
)

add_test(wasmedgeCommonTests wasmedgeCommonTests)

target_link_libraries(wasmedgeCommonTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeCommon
)

# On x86 with SSE2, the SIMD test changes the floating-point environment around
# the code that it checks, so the compiler must keep the floating-point work in
# that order. A compiler can warn that its strict option is unsupported on the
# target, so the option is used only when it compiles without a warning. A
# target without SSE2 has no environment checks and needs no option. Without
# the option, the test skips its environment checks.
if(MSVC)
  set(WASMEDGE_SIMD_TEST_STRICT_FP /fp:strict)
  set(WASMEDGE_SIMD_TEST_WERROR /WX)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(WASMEDGE_SIMD_TEST_STRICT_FP -frounding-math)
  set(WASMEDGE_SIMD_TEST_WERROR -Werror)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(WASMEDGE_SIMD_TEST_STRICT_FP -ffp-model=strict)
  set(WASMEDGE_SIMD_TEST_WERROR -Werror)
endif()
if(WASMEDGE_SIMD_TEST_STRICT_FP)
  include(CheckCXXSourceCompiles)
  include(CMakePushCheckState)
  # The results depend on the compiler flags, which can change in an existing
  # build directory, so the probes run again on each configuration.
  unset(WASMEDGE_SIMD_TEST_HAS_SSE2 CACHE)
  unset(WASMEDGE_SIMD_TEST_HAS_STRICT_FP CACHE)
  cmake_push_check_state(RESET)
  check_cxx_source_compiles("
    #if !defined(_M_X64) && !defined(__SSE2__)
    #error the test has no environment checks for this target
    #endif
    int main() { return 0; }"
    WASMEDGE_SIMD_TEST_HAS_SSE2
  )
  if(WASMEDGE_SIMD_TEST_HAS_SSE2)
    set(CMAKE_REQUIRED_FLAGS
      "${WASMEDGE_SIMD_TEST_STRICT_FP} ${WASMEDGE_SIMD_TEST_WERROR}"
    )
    check_cxx_source_compiles("int main() { return 0; }"
      WASMEDGE_SIMD_TEST_HAS_STRICT_FP
    )
  endif()
  cmake_pop_check_state()
  if(WASMEDGE_SIMD_TEST_HAS_STRICT_FP)
    set_source_files_properties(simdTest.cpp
      PROPERTIES
      COMPILE_OPTIONS "${WASMEDGE_SIMD_TEST_STRICT_FP}"
      COMPILE_DEFINITIONS WASMEDGE_SIMD_TEST_STRICT_FP
    )
  elseif(WASMEDGE_SIMD_TEST_HAS_SSE2)
    message(WARNING
      "${WASMEDGE_SIMD_TEST_STRICT_FP} gives a warning, so the SIMD test skips "
      "its floating-point environment checks."
    )
  endif()
endif()
