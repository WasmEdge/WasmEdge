# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

find_package(GTest QUIET)
if(GTest_FOUND)
  set(GTEST_BOTH_LIBRARIES "${GTEST_BOTH_LIBRARIES};Threads::Threads")
else()
  FetchContent_Declare(
    GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
  )
  set(BUILD_GMOCK OFF CACHE BOOL "Builds the googlemock subproject" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" FORCE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(
      ${WASMEDGE_CFLAGS}
    )
  else()
    add_compile_options(
      ${WASMEDGE_CFLAGS}
      -Wno-language-extension-token
      -Wno-missing-noreturn
      -Wno-shift-sign-overflow
      -Wno-undef
      -Wno-unused-member-function
      $<$<COMPILE_LANGUAGE:CXX>:-Wno-zero-as-null-pointer-constant>
      -Wno-deprecated
    )
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
      add_compile_options(
        -Wno-suggest-destructor-override
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-suggest-override>
      )
    endif()
  endif()

  FetchContent_MakeAvailable(GTest)
  set(GTEST_BOTH_LIBRARIES "gtest;gtest_main")
endif()

if(WASMEDGE_USE_LLVM)
  add_subdirectory(aot)
  add_subdirectory(llvm)
  add_subdirectory(mixcall)
endif()
add_subdirectory(common)
add_subdirectory(spec)
add_subdirectory(loader)
add_subdirectory(executor)
add_subdirectory(thread)
if(WASMEDGE_BUILD_SHARED_LIB)
  add_subdirectory(api)
  add_subdirectory(externref)
endif()
if(WASMEDGE_BUILD_PLUGINS)
  add_subdirectory(plugins)
endif()
add_subdirectory(host/socket)
add_subdirectory(host/wasi)
add_subdirectory(host/mock)
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
    EXCLUDE
    "${PROJECT_SOURCE_DIR}/thirdparty/*"
    "${PROJECT_SOURCE_DIR}/test/*"
  )
endif()
