# SPDX-License-Identifier: Apache-2.0
add_executable(ssvm
  main.cpp
)

target_link_libraries(ssvm
  PRIVATE
  ssvmVM
)
