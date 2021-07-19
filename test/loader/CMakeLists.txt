# SPDX-License-Identifier: Apache-2.0

add_executable(wasmedgeLoaderFileMgrTests
  filemgrTest.cpp
)

add_test(wasmedgeLoaderFileMgrTests wasmedgeLoaderFileMgrTests)

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/filemgrTestData
  ${CMAKE_CURRENT_BINARY_DIR}/filemgrTestData
  COPYONLY
)

target_link_libraries(wasmedgeLoaderFileMgrTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLoaderFileMgr
)
