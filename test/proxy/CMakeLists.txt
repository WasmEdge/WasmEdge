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
  ${CMAKE_CURRENT_SOURCE_DIR}/WasmTestData
  ${CMAKE_CURRENT_BINARY_DIR}/WasmTestData
  COPYONLY
)

target_link_libraries(ssvmProxyTests
  PRIVATE
  utilGoogleTest
  ssvmProxy
)
