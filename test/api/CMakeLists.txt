# SPDX-License-Identifier: Apache-2.0

if(WASMEDGE_DISABLE_AOT_RUNTIME)
  add_definitions(-DWASMEDGE_DISABLE_AOT_RUNTIME)
endif()

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/apiTestData
  ${CMAKE_CURRENT_BINARY_DIR}/apiTestData
  COPYONLY
)

add_library(wasmedgeAPITestHelpers
  hostfunc_c.c
  helper.cpp
)

target_include_directories(wasmedgeAPITestHelpers
  PUBLIC
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_SOURCE_DIR}/include
)

add_executable(wasmedgeAPIUnitTests
  APIUnitTest.cpp
)

add_test(wasmedgeAPIUnitTests wasmedgeAPIUnitTests)

target_link_libraries(wasmedgeAPIUnitTests
  PRIVATE
  std::filesystem
  utilGoogleTest
  wasmedgeAPITestHelpers
  wasmedge_c
)

add_executable(wasmedgeAPIVMCoreTests
  APIVMCoreTest.cpp
)

add_test(wasmedgeAPIVMCoreTests wasmedgeAPIVMCoreTests)

target_link_libraries(wasmedgeAPIVMCoreTests
  PRIVATE
  std::filesystem
  utilGoogleTest
  wasmedgeTestSpec
  wasmedgeAPITestHelpers
  wasmedge_c
)

add_executable(wasmedgeAPIStepsCoreTests
  APIStepsCoreTest.cpp
)

add_test(wasmedgeAPIStepsCoreTests wasmedgeAPIStepsCoreTests)

target_link_libraries(wasmedgeAPIStepsCoreTests
  PRIVATE
  std::filesystem
  utilGoogleTest
  wasmedgeTestSpec
  wasmedgeAPITestHelpers
  wasmedge_c
)

if (NOT WASMEDGE_DISABLE_AOT_RUNTIME)
  add_executable(wasmedgeAPIAOTCoreTests
    APIAOTCoreTest.cpp
  )
  add_test(wasmedgeAPIAOTCoreTests wasmedgeAPIAOTCoreTests)
  target_link_libraries(wasmedgeAPIAOTCoreTests
    PRIVATE
    std::filesystem
    utilGoogleTest
    wasmedgeTestSpec
    wasmedgeAPITestHelpers
    wasmedge_c
  )
endif()
