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

add_test(ssvmAOTCoreTests ssvmAOTCoreTests)

target_link_libraries(ssvmAOTCoreTests
  PRIVATE
  std::filesystem
  utilGoogleTest
  ssvmTestSpec
  ssvmLoader
  ssvmAOT
  ssvmVM
)

add_executable(ssvmAOTBlake3Tests
  AOTBlake3Test.cpp
)

add_test(ssvmAOTBlake3Tests ssvmAOTBlake3Tests)

target_link_libraries(ssvmAOTBlake3Tests
  PRIVATE
  utilGoogleTest
  ssvmAOT
)
