# SPDX-License-Identifier: Apache-2.0

add_library(wasmedgeAST
  module.cpp
  section.cpp
  description.cpp
  segment.cpp
  type.cpp
  expression.cpp
  instruction.cpp
)

target_include_directories(wasmedgeAST
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(wasmedgeAST
  PUBLIC
  wasmedgeLoaderFileMgr
  wasmedgeCommon
)
