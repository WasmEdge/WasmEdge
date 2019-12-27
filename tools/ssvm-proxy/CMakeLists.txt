# SPDX-License-Identifier: Apache-2.0
configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/ethereum
  ${CMAKE_CURRENT_BINARY_DIR}/ethereum
  COPYONLY
)

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/inputJSON
  ${CMAKE_CURRENT_BINARY_DIR}/inputJSON
  COPYONLY
)

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/outputJSON
  ${CMAKE_CURRENT_BINARY_DIR}/outputJSON
  COPYONLY
)

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/wasm
  ${CMAKE_CURRENT_BINARY_DIR}/wasm
  COPYONLY
)

add_executable(ssvm-proxy
  main.cpp
)

target_link_libraries(ssvm-proxy
  PRIVATE
  ssvmProxy
)
