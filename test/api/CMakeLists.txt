# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

if(WASMEDGE_USE_LLVM)
  add_definitions(-DWASMEDGE_USE_LLVM)
endif()
if(WASMEDGE_BUILD_PLUGINS)
  add_definitions(-DWASMEDGE_BUILD_PLUGINS)
endif()

file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/apiTestData
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

wasmedge_add_executable(wasmedgeAPIWasmCUnitTests
  APIWasmCUnitTest.cpp
)

add_test(wasmedgeAPIWasmCUnitTests wasmedgeAPIWasmCUnitTests)

target_include_directories(wasmedgeAPIWasmCUnitTests
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty/wasm
)

target_link_libraries(wasmedgeAPIWasmCUnitTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedge_shared
)

wasmedge_add_library(wasmedgeAPITestHelpers
  hostfunc_c.c
  helper.cpp
)

target_link_libraries(wasmedgeAPITestHelpers
  PUBLIC
  wasmedge_shared
  wasmedgeTestSpec
)

wasmedge_add_executable(wasmedgeAPIUnitTests
  APIUnitTest.cpp
)

add_test(wasmedgeAPIUnitTests wasmedgeAPIUnitTests)

target_link_libraries(wasmedgeAPIUnitTests
  PRIVATE
  wasmedgeAPITestHelpers
  wasmedge_shared
)

target_include_directories(wasmedgeAPIUnitTests
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

wasmedge_add_executable(wasmedgeAPIVMCoreTests
  APIVMCoreTest.cpp
)

add_test(wasmedgeAPIVMCoreTests wasmedgeAPIVMCoreTests)

target_link_libraries(wasmedgeAPIVMCoreTests
  PRIVATE
  wasmedgeAPITestHelpers
  wasmedge_shared
)

wasmedge_add_executable(wasmedgeAPIStepsCoreTests
  APIStepsCoreTest.cpp
)

add_test(wasmedgeAPIStepsCoreTests wasmedgeAPIStepsCoreTests)

target_link_libraries(wasmedgeAPIStepsCoreTests
  PRIVATE
  wasmedgeAPITestHelpers
  wasmedge_shared
)

if(WASMEDGE_USE_LLVM)
  wasmedge_add_executable(wasmedgeAPIAOTCoreTests
    APIAOTCoreTest.cpp
  )

  add_test(wasmedgeAPIAOTCoreTests wasmedgeAPIAOTCoreTests)

  target_link_libraries(wasmedgeAPIAOTCoreTests
    PRIVATE
    wasmedgeAPITestHelpers
    wasmedge_shared
  )

  wasmedge_add_executable(wasmedgeAPIAOTNestedVMTests
    APIAOTNestedVMTest.cpp
  )

  add_test(wasmedgeAPIAOTNestedVMTests wasmedgeAPIAOTNestedVMTests)

  target_link_libraries(wasmedgeAPIAOTNestedVMTests
    PRIVATE
    wasmedgeAPITestHelpers
    wasmedge_shared
  )
endif()
