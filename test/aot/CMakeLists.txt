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
