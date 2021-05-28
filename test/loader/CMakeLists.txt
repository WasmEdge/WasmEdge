# SPDX-License-Identifier: Apache-2.0

add_executable(wasmedgeLoaderFileMgrTests
  filemgrTest.cpp
)

add_test(wasmedgeLoaderFileMgrTests wasmedgeLoaderFileMgrTests)

add_executable(wasmedgeLoaderWagonTests
  wagonTest.cpp
)

add_test(wasmedgeLoaderWagonTests wasmedgeLoaderWagonTests)

add_executable(wasmedgeLoaderEthereumTests
  ethereumTest.cpp
)

add_test(wasmedgeLoaderEthereumTests wasmedgeLoaderEthereumTests)

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/filemgrTestData
  ${CMAKE_CURRENT_BINARY_DIR}/filemgrTestData
  COPYONLY
)
configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/wagonTestData
  ${CMAKE_CURRENT_BINARY_DIR}/wagonTestData
  COPYONLY
)
configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/ethereumTestData
  ${CMAKE_CURRENT_BINARY_DIR}/ethereumTestData
  COPYONLY
)

target_link_libraries(wasmedgeLoaderFileMgrTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLoaderFileMgr
)

target_link_libraries(wasmedgeLoaderWagonTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLoaderFileMgr
  wasmedgeAST
)

target_link_libraries(wasmedgeLoaderEthereumTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLoaderFileMgr
  wasmedgeAST
)
