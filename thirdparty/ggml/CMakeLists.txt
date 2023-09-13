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

target_compile_options(utilGgml
  PRIVATE
  -Wno-unused-parameter
  -Wno-unused-variable
  -Wno-unused-but-set-variable
  -Wno-unused-function
  -Wno-missing-braces
  -DGGML_USE_K_QUANTS
  -DGGML_USE_OPENBLAS
)

if(WIN32)
  target_compile_options(utilGgml
    PRIVATE
    -Wno-string-conversion
    -Wno-sign-conversion
    -Wno-macro-redefined
    -Wno-missing-prototypes
    -Wno-unreachable-code-return
    -Wno-shorten-64-to-32
    -Wno-implicit-int-conversion
    -Wno-implicit-float-conversion
    -Wno-float-conversion
    -Wno-unused-macros
    -Wno-unreachable-code-break
    -Wno-cast-align
    -Wno-undef
    -Wno-shadow-uncaptured-local
    -Wno-unreachable-code
    -Wno-cast-function-type
    -Wno-format-nonliteral
    -Wno-extra-semi-stmt
    -Wno-bad-function-cast
  )
endif()

target_include_directories(utilGgml
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
