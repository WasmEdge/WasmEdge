# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

include(FetchContent)

wasmedge_setup_simdjson()

message(STATUS "Downloading the WASM spec test suite")
FetchContent_Declare(
  wasmedge_unit_test
  GIT_REPOSITORY https://github.com/WasmEdge/wasmedge-spectest
  GIT_TAG        89424e4d14fa4f101804c27333d2065d2a2b3f23  # wasm-core-20260922
)
FetchContent_MakeAvailable(wasmedge_unit_test)
message(STATUS "Downloading the WASM spec test suite -- done")

function(wasmedge_copy_spec_testsuite proposal)
  message(STATUS "Copying test suite to ${CMAKE_CURRENT_BINARY_DIR}/testSuites/${proposal}")
  file(COPY
    ${wasmedge_unit_test_SOURCE_DIR}/${proposal}
    DESTINATION
    ${CMAKE_CURRENT_BINARY_DIR}/testSuites
  )
  message(STATUS "Copying test suite to ${CMAKE_CURRENT_BINARY_DIR}/testSuites/${proposal} -- done")
endfunction()

set(WASMEDGE_SPECTEST_FOLDERS
  "wasm-1.0"
  "wasm-2.0"
  "wasm-3.0"
  "wasm-3.0-bulk-memory"
  "wasm-3.0-exceptions"
  "wasm-3.0-gc"
  "wasm-3.0-memory64"
  "wasm-3.0-multi-memory"
  "wasm-3.0-relaxed-simd"
  "wasm-3.0-simd"
  "threads"
  "component-model-wasm-tools"
)

foreach(PROPOSAL ${WASMEDGE_SPECTEST_FOLDERS})
  wasmedge_copy_spec_testsuite(${PROPOSAL})
endforeach()

wasmedge_add_library(wasmedgeTestSpec
  spectest.cpp
)

target_link_libraries(wasmedgeTestSpec
  PRIVATE
  simdjson::simdjson
  PUBLIC
  std::filesystem
  wasmedgeCommon
  ${GTEST_BOTH_LIBRARIES}
)

wasmedge_add_executable(wasmedgeSpecInterpreterTests
  SpecInterpreterTest.cpp
)

add_test(wasmedgeSpecInterpreterTests wasmedgeSpecInterpreterTests)

target_link_libraries(wasmedgeSpecInterpreterTests
  PRIVATE
  wasmedgeTestSpec
  wasmedgeVM
)

if(WASMEDGE_USE_LLVM)
  wasmedge_add_executable(wasmedgeSpecAOTTests
    SpecAOTTest.cpp
  )

  add_test(wasmedgeSpecAOTTests wasmedgeSpecAOTTests)

  target_link_libraries(wasmedgeSpecAOTTests
    PRIVATE
    wasmedgeTestSpec
    wasmedgeVM
  )

  wasmedge_add_executable(wasmedgeSpecJITTests
    SpecJITTest.cpp
  )

  add_test(wasmedgeSpecJITTests wasmedgeSpecJITTests)

  target_link_libraries(wasmedgeSpecJITTests
    PRIVATE
    wasmedgeTestSpec
    wasmedgeVM
  )
endif()

if(WASMEDGE_BUILD_SHARED_LIB)
  wasmedge_add_library(wasmedgeTestSpecAPI
    apihelper.cpp
    hostfunc_c.c
  )

  target_link_libraries(wasmedgeTestSpecAPI
    PUBLIC
    wasmedge_shared
    wasmedgeTestSpec
  )

  wasmedge_add_executable(wasmedgeSpecAPIVMTests
    SpecAPIVMTest.cpp
  )

  add_test(wasmedgeSpecAPIVMTests wasmedgeSpecAPIVMTests)

  target_link_libraries(wasmedgeSpecAPIVMTests
    PRIVATE
    wasmedgeTestSpecAPI
    wasmedge_shared
  )

  wasmedge_add_executable(wasmedgeSpecAPIStepsTests
    SpecAPIStepsTest.cpp
  )

  add_test(wasmedgeSpecAPIStepsTests wasmedgeSpecAPIStepsTests)

  target_link_libraries(wasmedgeSpecAPIStepsTests
    PRIVATE
    wasmedgeTestSpecAPI
    wasmedge_shared
  )

  if(WASMEDGE_USE_LLVM)
    wasmedge_add_executable(wasmedgeSpecAPIAOTTests
      SpecAPIAOTTest.cpp
    )

    add_test(wasmedgeSpecAPIAOTTests wasmedgeSpecAPIAOTTests)

    target_link_libraries(wasmedgeSpecAPIAOTTests
      PRIVATE
      wasmedgeTestSpecAPI
      wasmedge_shared
    )

    set_tests_properties(wasmedgeSpecAOTTests wasmedgeSpecAPIAOTTests
      PROPERTIES
      RESOURCE_LOCK wasmedgeSpecTestSuites
    )
  endif()
endif()
