# SPDX-License-Identifier: Apache-2.0

add_library(ssvmAST
  module.cpp
  section.cpp
  description.cpp
  segment.cpp
  type.cpp
  expression.cpp
  instruction.cpp
)

target_include_directories(ssvmAST
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
