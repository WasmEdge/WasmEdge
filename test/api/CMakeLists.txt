# SPDX-License-Identifier: Apache-2.0

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/apiTestData
  ${CMAKE_CURRENT_BINARY_DIR}/apiTestData
  COPYONLY
)

add_executable(ssvmAPIUnitTests
  APIUnitTest.cpp
)

add_test(ssvmAPIUnitTests ssvmAPIUnitTests)

target_link_libraries(ssvmAPIUnitTests
  PRIVATE
  std::filesystem
  utilGoogleTest
  ssvm_c
)

add_executable(ssvmAPIVMCoreTests
  hostfunc_c.c
  helper.cpp
  APIVMCoreTest.cpp
)

add_test(ssvmAPIVMCoreTests ssvmAPIVMCoreTests)

target_link_libraries(ssvmAPIVMCoreTests
  PRIVATE
  std::filesystem
  utilGoogleTest
  ssvmTestSpec
  ssvm_c
)

add_executable(ssvmAPIStepsCoreTests
  hostfunc_c.c
  helper.cpp
  APIStepsCoreTest.cpp
)

add_test(ssvmAPIStepsCoreTests ssvmAPIStepsCoreTests)

target_link_libraries(ssvmAPIStepsCoreTests
  PRIVATE
  std::filesystem
  utilGoogleTest
  ssvmTestSpec
  ssvm_c
)
