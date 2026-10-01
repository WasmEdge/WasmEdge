# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

wasmedge_add_executable(wasmedgeAPITests
  APIExecutorRegressionTest.cpp
  APIInterruptTest.cpp
  APIUnitTest.cpp
)

add_test(wasmedgeAPITests wasmedgeAPITests)

target_link_libraries(wasmedgeAPITests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  std::filesystem
  wasmedge_shared
)

target_include_directories(wasmedgeAPITests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

wasmedge_add_executable(wasmedgeAPIWasmCTests
  APIWasmCExampleTest.cpp
  APIWasmCRegressionTest.cpp
  APIWasmCUnitTest.cpp
)

add_test(wasmedgeAPIWasmCTests wasmedgeAPIWasmCTests)

target_include_directories(wasmedgeAPIWasmCTests
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty/wasm
)

target_link_libraries(wasmedgeAPIWasmCTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedge_shared
)

if(WASMEDGE_USE_LLVM)
  wasmedge_add_executable(wasmedgeAPIAOTTests
    APIAOTNestedVMTest.cpp
  )

  add_test(wasmedgeAPIAOTTests wasmedgeAPIAOTTests)

  target_link_libraries(wasmedgeAPIAOTTests
    PRIVATE
    ${GTEST_BOTH_LIBRARIES}
    wasmedge_shared
  )
endif()
