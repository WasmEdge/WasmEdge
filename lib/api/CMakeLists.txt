# SPDX-License-Identifier: Apache-2.0

add_library(ssvm_c SHARED
  ssvm.cpp
)

target_link_libraries(ssvm_c
  PUBLIC
  ssvmVM
)

target_include_directories(ssvm_c
  PUBLIC
  ${PROJECT_BINARY_DIR}/include
)