# SPDX-License-Identifier: Apache-2.0
add_executable(ssvmProxyTests
  proxyTest.cpp
)

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/inputJSONTestData
  ${CMAKE_CURRENT_BINARY_DIR}/inputJSONTestData
  COPYONLY
)

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/outputJSONTestData
  ${CMAKE_CURRENT_BINARY_DIR}/outputJSONTestData
  COPYONLY
)

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/bytecodeTestData
  ${CMAKE_CURRENT_BINARY_DIR}/bytecodeTestData
  COPYONLY
)

target_link_libraries(ssvmProxyTests
  PRIVATE
  utilGoogleTest
  utilKeccak
  ssvmVM
  ssvmProxy
)
