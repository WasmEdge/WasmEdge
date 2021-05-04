# SPDX-License-Identifier: Apache-2.0

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/apiTestData
  ${CMAKE_CURRENT_BINARY_DIR}/apiTestData
  COPYONLY
)

add_library(ssvmAPITestHelpers
  hostfunc_c.c
  helper.cpp
)

target_include_directories(ssvmAPITestHelpers
  PUBLIC
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_SOURCE_DIR}/include
)

add_executable(ssvmAPIUnitTests
  APIUnitTest.cpp
)

add_test(ssvmAPIUnitTests ssvmAPIUnitTests)

target_link_libraries(ssvmAPIUnitTests
  PRIVATE
  std::filesystem
  utilGoogleTest
  ssvmAPITestHelpers
  ssvm_c
)

add_executable(ssvmAPIVMCoreTests
  APIVMCoreTest.cpp
)

add_test(ssvmAPIVMCoreTests ssvmAPIVMCoreTests)

target_link_libraries(ssvmAPIVMCoreTests
  PRIVATE
  std::filesystem
  utilGoogleTest
  ssvmTestSpec
  ssvmAPITestHelpers
  ssvm_c
)

add_executable(ssvmAPIStepsCoreTests
  APIStepsCoreTest.cpp
)

add_test(ssvmAPIStepsCoreTests ssvmAPIStepsCoreTests)

target_link_libraries(ssvmAPIStepsCoreTests
  PRIVATE
  std::filesystem
  utilGoogleTest
  ssvmTestSpec
  ssvmAPITestHelpers
  ssvm_c
)

if (NOT SSVM_DISABLE_AOT_RUNTIME)
  add_executable(ssvmAPIAOTCoreTests
    APIAOTCoreTest.cpp
  )
  add_test(ssvmAPIAOTCoreTests ssvmAPIAOTCoreTests)
  target_link_libraries(ssvmAPIAOTCoreTests
    PRIVATE
    std::filesystem
    utilGoogleTest
    ssvmTestSpec
    ssvmAPITestHelpers
    ssvm_c
  )
endif()