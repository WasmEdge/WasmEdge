# SPDX-License-Identifier: Apache-2.0
configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/examples
  ${CMAKE_CURRENT_BINARY_DIR}/examples
  COPYONLY
)

add_executable(ssvm
  main.cpp
)

install(TARGETS ssvm EXPORT ssvm DESTINATION bin)

target_link_libraries(ssvm
  PRIVATE
  ssvmVM
)
