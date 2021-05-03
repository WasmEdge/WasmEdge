# SPDX-License-Identifier: Apache-2.0

add_library(wasmedge_c SHARED
  wasmedge.cpp
)

target_link_libraries(wasmedge_c
  PUBLIC
  wasmedgeAOT
  wasmedgeVM
)

target_include_directories(wasmedge_c
  PUBLIC
  ${PROJECT_BINARY_DIR}/include
)
