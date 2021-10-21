# SPDX-License-Identifier: Apache-2.0
wasmedge_add_library(wasmedgeSignature
  signature.cpp
  sig_algorithm.cpp
)

target_include_directories(wasmedgeSignature
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(wasmedgeSignature
  PUBLIC
  wasmedgeCommon
  wasmedgeAST
)
