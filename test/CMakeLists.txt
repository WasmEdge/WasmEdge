# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

find_package(GTest QUIET)
if(GTest_FOUND)
  set(GTEST_BOTH_LIBRARIES "${GTEST_BOTH_LIBRARIES};Threads::Threads")
  wasmedge_mark_system_includes(GTest::gtest)
  wasmedge_mark_system_includes(GTest::gtest_main)
else()
  FetchContent_Declare(
    GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 52eb8108c5bdec04579160ae17225d66034bd723  # v1.17.0
    GIT_SHALLOW TRUE
  )
  set(BUILD_GMOCK OFF CACHE BOOL "Builds the googlemock subproject" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" FORCE)
  # clang-cl maps -Wall to -Weverything, so googletest and the WasmEdge test
  # sources it is linked into surface warnings no other compiler enables. Every
  # other compiler builds them cleanly under WASMEDGE_CFLAGS, so this is scoped to
  # clang-cl only. These stay -Wno-error (visible but non-fatal) rather than fully
  # silenced, and the list is trimmed to the warnings that actually fire.
  if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
      ${WASMEDGE_CFLAGS}
      -Wno-error=character-conversion
      -Wno-error=missing-variable-declarations
      -Wno-error=nrvo
      -Wno-error=suggest-override
      -Wno-error=unused-member-function
    )
  endif()
  FetchContent_MakeAvailable(GTest)
  wasmedge_mark_system_includes(gtest)
  wasmedge_mark_system_includes(gtest_main)
  wasmedge_suppress_shadow_warnings(gtest gtest_main)
  set(GTEST_BOTH_LIBRARIES "gtest_main")
endif()

if(WASMEDGE_USE_LLVM)
  add_subdirectory(aot)
  add_subdirectory(llvm)
  add_subdirectory(mixcall)
endif()
add_subdirectory(common)
add_subdirectory(wat)
add_subdirectory(spec)
add_subdirectory(loader)
add_subdirectory(executor)
add_subdirectory(validator)
add_subdirectory(component)
add_subdirectory(thread)
if(WASMEDGE_BUILD_SHARED_LIB)
  add_subdirectory(api)
  add_subdirectory(driver)
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
add_subdirectory(instance)
add_subdirectory(errinfo)
add_subdirectory(vm)

if(WASMEDGE_BUILD_COVERAGE)
  include(ProcessorCount)
  ProcessorCount(PROCESSOR_COUNT)
  if(PROCESSOR_COUNT EQUAL 0)
    set(PROCESSOR_COUNT 1)
  endif()
  set(GCOVR_ADDITIONAL_ARGS --gcov-ignore-parse-errors negative_hits.warn_once_per_file)
  setup_target_for_coverage_gcovr_html(
    NAME coverage
    EXECUTABLE ctest -j ${PROCESSOR_COUNT} --output-on-failure
    DEPENDENCIES wasmedge
    BASE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    EXCLUDE
    "${PROJECT_SOURCE_DIR}/thirdparty/*"
    "${PROJECT_SOURCE_DIR}/test/*"
  )
  setup_target_for_coverage_gcovr_xml(
    NAME codecov
    EXECUTABLE ctest -j ${PROCESSOR_COUNT} --output-on-failure
    DEPENDENCIES wasmedge
    EXCLUDE
    "${PROJECT_SOURCE_DIR}/thirdparty/*"
    "${PROJECT_SOURCE_DIR}/test/*"
  )
endif()
