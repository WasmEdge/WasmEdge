# SPDX-License-Identifier: Apache-2.0

if(WASMEDGE_BUILD_AOT_RUNTIME)
  add_definitions(-DWASMEDGE_BUILD_AOT_RUNTIME)
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
  wasmedge_c
)

wasmedge_add_library(wasmedgeAPITestHelpers
  hostfunc_c.c
  helper.cpp
)

target_link_libraries(wasmedgeAPITestHelpers
  PUBLIC
  wasmedge_c
)

wasmedge_add_executable(wasmedgeAPIUnitTests
  APIUnitTest.cpp
)

add_test(wasmedgeAPIUnitTests wasmedgeAPIUnitTests)

target_link_libraries(wasmedgeAPIUnitTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeAPITestHelpers
  wasmedge_c
)

wasmedge_add_executable(wasmedgeAPIVMCoreTests
  APIVMCoreTest.cpp
)

add_test(wasmedgeAPIVMCoreTests wasmedgeAPIVMCoreTests)

target_link_libraries(wasmedgeAPIVMCoreTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeTestSpec
  wasmedgeAPITestHelpers
  wasmedge_c
)

wasmedge_add_executable(wasmedgeAPIStepsCoreTests
  APIStepsCoreTest.cpp
)

add_test(wasmedgeAPIStepsCoreTests wasmedgeAPIStepsCoreTests)

target_link_libraries(wasmedgeAPIStepsCoreTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeTestSpec
  wasmedgeAPITestHelpers
  wasmedge_c
)

if (WASMEDGE_BUILD_AOT_RUNTIME)
  wasmedge_add_executable(wasmedgeAPIAOTCoreTests
    APIAOTCoreTest.cpp
  )

  add_test(wasmedgeAPIAOTCoreTests wasmedgeAPIAOTCoreTests)

  target_link_libraries(wasmedgeAPIAOTCoreTests
    PRIVATE
    std::filesystem
    ${GTEST_BOTH_LIBRARIES}
    wasmedgeTestSpec
    wasmedgeAPITestHelpers
    wasmedge_c
  )
endif()
