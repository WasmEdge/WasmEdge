# SPDX-License-Identifier: Apache-2.0

add_executable(ssvmLoaderFileMgrTests
  filemgrTest.cpp
)

add_test(ssvmLoaderFileMgrTests ssvmLoaderFileMgrTests)

add_executable(ssvmLoaderWagonTests
  wagonTest.cpp
)

add_test(ssvmLoaderWagonTests ssvmLoaderWagonTests)

add_executable(ssvmLoaderEthereumTests
  ethereumTest.cpp
)

add_test(ssvmLoaderEthereumTests ssvmLoaderEthereumTests)

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

target_link_libraries(ssvmLoaderFileMgrTests
  PRIVATE
  utilGoogleTest
  ssvmLoaderFileMgr
)

target_link_libraries(ssvmLoaderWagonTests
  PRIVATE
  utilGoogleTest
  ssvmLoaderFileMgr
  ssvmAST
)

target_link_libraries(ssvmLoaderEthereumTests
  PRIVATE
  utilGoogleTest
  ssvmLoaderFileMgr
  ssvmAST
)
