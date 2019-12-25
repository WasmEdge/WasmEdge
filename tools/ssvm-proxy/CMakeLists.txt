# SPDX-License-Identifier: Apache-2.0
configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/ethereum
  ${CMAKE_CURRENT_BINARY_DIR}/ethereum
  COPYONLY
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

add_executable(ssvm-proxy
  main.cpp
)

target_link_libraries(ssvm-proxy
  PRIVATE
  ssvmVM
  ssvmProxy
)
