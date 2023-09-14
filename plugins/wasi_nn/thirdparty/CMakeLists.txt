# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2023 Second State INC

if(WASMEDGE_PLUGIN_WASI_NN_BACKEND)
  string(TOLOWER ${WASMEDGE_PLUGIN_WASI_NN_BACKEND} BACKEND)
  if(BACKEND STREQUAL "ggml")
  add_compile_options(-DGGML_BACKEND)
    add_subdirectory(ggml)
  endif()
endif()
