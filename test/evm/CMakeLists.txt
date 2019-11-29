# SPDX-License-Identifier: Apache-2.0

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/ethereumTestData
  ${CMAKE_CURRENT_BINARY_DIR}/ethereumTestData
  COPYONLY
)

add_executable(ssvmEVMTests
  erc20Test.cpp
)

target_link_libraries(ssvmEVMTests
  PRIVATE
  utilGoogleTest
  utilKeccak
  ssvmVM
)
