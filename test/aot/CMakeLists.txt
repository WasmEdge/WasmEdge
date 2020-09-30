# SPDX-License-Identifier: Apache-2.0

add_executable(ssvmAOTWagonTests
  AOTwagonTest.cpp
)

add_test(ssvmAOTWagonTests ssvmAOTWagonTests)

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/../loader/wagonTestData
  ${CMAKE_CURRENT_BINARY_DIR}/../loader/wagonTestData
  COPYONLY
)

target_link_libraries(ssvmAOTWagonTests
  PRIVATE
  utilGoogleTest
  ssvmLoader
  ssvmAOT
)

add_executable(ssvmAOTCoreTests
  AOTcoreTest.cpp
)

### TODO: Enable this for CI.
#add_test(ssvmAOTCoreTests ssvmAOTCoreTests)

target_link_libraries(ssvmAOTCoreTests
  PRIVATE
  std::filesystem
  utilGoogleTest
  ssvmTestSpec
  ssvmLoader
  ssvmAOT
  ssvmVM
)
