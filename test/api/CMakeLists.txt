# SPDX-License-Identifier: Apache-2.0

if(BUILD_AOT_RUNTIME)
  add_definitions(-DBUILD_AOT_RUNTIME)
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

target_compile_options(wasmedgeAPITestHelpers
  PRIVATE
  ${WASMEDGE_CFLAGS}
)

target_link_libraries(wasmedgeAPITestHelpers
  PUBLIC
  wasmedge_c
)

add_executable(wasmedgeAPIUnitTests
  APIUnitTest.cpp
)

add_test(wasmedgeAPIUnitTests wasmedgeAPIUnitTests)

target_compile_options(wasmedgeAPIUnitTests
  PRIVATE
  ${WASMEDGE_CFLAGS}
)

target_link_libraries(wasmedgeAPIUnitTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeAPITestHelpers
  wasmedge_c
)

add_executable(wasmedgeAPIVMCoreTests
  APIVMCoreTest.cpp
)

add_test(wasmedgeAPIVMCoreTests wasmedgeAPIVMCoreTests)

target_compile_options(wasmedgeAPIVMCoreTests
  PRIVATE
  ${WASMEDGE_CFLAGS}
)

target_link_libraries(wasmedgeAPIVMCoreTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeTestSpec
  wasmedgeAPITestHelpers
  wasmedge_c
)

add_executable(wasmedgeAPIStepsCoreTests
  APIStepsCoreTest.cpp
)

add_test(wasmedgeAPIStepsCoreTests wasmedgeAPIStepsCoreTests)

target_compile_options(wasmedgeAPIStepsCoreTests
  PRIVATE
  ${WASMEDGE_CFLAGS}
)

target_link_libraries(wasmedgeAPIStepsCoreTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeTestSpec
  wasmedgeAPITestHelpers
  wasmedge_c
)

if (BUILD_AOT_RUNTIME)
  add_executable(wasmedgeAPIAOTCoreTests
    APIAOTCoreTest.cpp
  )

  add_test(wasmedgeAPIAOTCoreTests wasmedgeAPIAOTCoreTests)

  target_compile_options(wasmedgeAPIAOTCoreTests
    PRIVATE
    ${WASMEDGE_CFLAGS}
  )

  target_link_libraries(wasmedgeAPIAOTCoreTests
    PRIVATE
    std::filesystem
    ${GTEST_BOTH_LIBRARIES}
    wasmedgeTestSpec
    wasmedgeAPITestHelpers
    wasmedge_c
  )
endif()
