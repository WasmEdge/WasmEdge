# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

if(WASMEDGE_BUILD_AOT_RUNTIME)
  add_definitions(-DWASMEDGE_BUILD_AOT_RUNTIME)
endif()
if(WASMEDGE_BUILD_PLUGINS)
  add_definitions(-DWASMEDGE_BUILD_PLUGINS)
endif()

file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/apiTestData
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

wasmedge_add_library(wasmedgeAPITestHelpers
  hostfunc_c.c
  helper.cpp
)

target_link_libraries(wasmedgeAPITestHelpers
  PRIVATE
  wasmedge_shared
  wasmedgeTestSpec
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
  wasmedge_shared
)

target_include_directories(wasmedgeAPIUnitTests
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

# Build the APIMallocTest

wasmedge_add_executable(wasmedgeAPIMallocTest
  APIMallocTest.cpp
)

add_test(wasmedgeAPIMallocTest wasmedgeAPIMallocTest)

target_link_libraries(wasmedgeAPIMallocTest
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeTestSpec
  wasmedgeAPITestHelpers
  wasmedge_shared
)

target_include_directories(wasmedgeAPIMallocTest
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

# Add custom command to convert cmalloc.wat to cmalloc.wasm
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/apiTestData/cmalloc.wasm
    COMMAND wat2wasm ${CMAKE_CURRENT_BINARY_DIR}/apiTestData/cmalloc.wat
                    -o ${CMAKE_CURRENT_BINARY_DIR}/apiTestData/cmalloc.wasm
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/apiTestData/cmalloc.wat
)

# Mark the generated cmalloc.wasm as a custom target
add_custom_target(cmalloc_wasm_target DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/apiTestData/cmalloc.wasm)

# Make sure the target is built before the unit tests
add_dependencies(wasmedgeAPIMallocTest cmalloc_wasm_target)

############################################################

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
  wasmedge_shared
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
  wasmedge_shared
)

if(WASMEDGE_BUILD_AOT_RUNTIME)
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
    wasmedge_shared
  )
endif()
