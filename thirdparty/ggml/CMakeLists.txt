# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(utilGgml
  common.cpp
  ggml-alloc.c
  ggml.c
  k_quants.c
  ggml-alloc.c
  llama.cpp
)

target_include_directories(utilGgml
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
