# SPDX-License-Identifier: Apache-2.0

find_package(GTest QUIET)
if(GTest_FOUND)
  set(GTEST_BOTH_LIBRARIES "${GTEST_BOTH_LIBRARIES};Threads::Threads")
else()
  FetchContent_Declare(
    GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.10.0
    GIT_SHALLOW    TRUE
  )
  set(BUILD_GMOCK OFF)
  get_property(
    compile_options
    DIRECTORY
    PROPERTY COMPILE_OPTIONS
  )
  set_property(
    DIRECTORY
    APPEND
    PROPERTY COMPILE_OPTIONS
    ${WASMEDGE_CFLAGS}
    -Wno-language-extension-token
    -Wno-missing-noreturn
    -Wno-shift-sign-overflow
    -Wno-undef
    -Wno-unused-member-function
    -Wno-zero-as-null-pointer-constant
  )
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set_property(
      DIRECTORY
      APPEND
      PROPERTY COMPILE_OPTIONS
      -Wno-suggest-destructor-override
      -Wno-suggest-override
    )
  endif()
  FetchContent_MakeAvailable(GTest)
  set_property(
    DIRECTORY
    PROPERTY COMPILE_OPTIONS
    ${compile_options}
  )
  unset(compile_options)
  set(GTEST_BOTH_LIBRARIES "gtest;gtest_main")
endif()

if (WASMEDGE_BUILD_AOT_RUNTIME)
  add_subdirectory(aot)
endif()
add_subdirectory(spec)
add_subdirectory(loader)
add_subdirectory(interpreter)
if (WASMEDGE_BUILD_SHARED_LIB)
  add_subdirectory(api)
endif()
add_subdirectory(host/wasmedge_process)
add_subdirectory(host/wasi)
add_subdirectory(externref)
add_subdirectory(expected)
add_subdirectory(span)
add_subdirectory(po)
add_subdirectory(memlimit)
add_subdirectory(errinfo)

if(WASMEDGE_BUILD_COVERAGE)
  setup_target_for_coverage_gcovr_html(
    NAME coverage
    EXECUTABLE ctest -j ${PROCESSOR_COUNT}
    DEPENDENCIES wasmedge
    BASE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    EXCLUDE
      "${PROJECT_SOURCE_DIR}/thirdparty/*"
      "${PROJECT_SOURCE_DIR}/test/*"
    )
  setup_target_for_coverage_gcovr_xml(
    NAME codecov
    EXECUTABLE ctest -j ${PROCESSOR_COUNT}
    DEPENDENCIES wasmedge
    BASE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    EXCLUDE
      "${PROJECT_SOURCE_DIR}/thirdparty/*"
      "${PROJECT_SOURCE_DIR}/test/*"
    )

endif()
