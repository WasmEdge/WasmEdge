# SPDX-License-Identifier: Apache-2.0

add_executable(ssvmExternrefTests
  ExternrefTest.cpp
)

add_test(ssvmExternrefTests ssvmExternrefTests)

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/externrefTestData
  ${CMAKE_CURRENT_BINARY_DIR}/externrefTestData
  COPYONLY
)

target_link_libraries(ssvmExternrefTests
  PRIVATE
  utilGoogleTest
  ssvmVM
)
