# SPDX-License-Identifier: Apache-2.0

add_executable(wasmedgeExternrefTests
  ExternrefTest.cpp
)

add_test(wasmedgeExternrefTests wasmedgeExternrefTests)

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/externrefTestData
  ${CMAKE_CURRENT_BINARY_DIR}/externrefTestData
  COPYONLY
)

target_link_libraries(wasmedgeExternrefTests
  PRIVATE
  utilGoogleTest
  wasmedgeVM
)
