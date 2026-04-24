# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2026 Second State INC

# Vendored tree-sitter runtime (MIT license, v0.26.6)
set(TREE_SITTER_DIR ${PROJECT_SOURCE_DIR}/thirdparty/tree-sitter)
# Generated tree-sitter WAT grammar parser
set(TREE_SITTER_WAT_DIR ${PROJECT_SOURCE_DIR}/thirdparty/tree-sitter-wat)

# C++ wrapper and parser
wasmedge_add_library(wasmedgeWat
  tree_sitter.cpp
  parser.cpp
  converter.cpp
  convert_types.cpp
  convert_instrs.cpp
  convert_sections.cpp
  maybe_wat.cpp
)

# Build tree-sitter runtime + grammar as a separate C object library
add_library(wasmedgeWatTreeSitter OBJECT
  ${TREE_SITTER_DIR}/lib.c
  ${TREE_SITTER_WAT_DIR}/src/parser.c
  ${TREE_SITTER_WAT_DIR}/src/scanner.c
)
set_target_properties(wasmedgeWatTreeSitter PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED ON
  POSITION_INDEPENDENT_CODE ON
)
target_include_directories(wasmedgeWatTreeSitter
  PRIVATE
  ${TREE_SITTER_WAT_DIR}/src
  ${TREE_SITTER_DIR}
)

target_link_libraries(wasmedgeWat
  PRIVATE
  wasmedgeWatTreeSitter
  PUBLIC
  wasmedgeCommon
)

target_include_directories(wasmedgeWat
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${TREE_SITTER_DIR}
)

# Custom target for grammar regeneration (developers only)
add_custom_target(wasmedge_generate_wat_grammar
  COMMAND npm install
  COMMAND npx tree-sitter generate grammar.js
  WORKING_DIRECTORY ${TREE_SITTER_WAT_DIR}
  COMMENT "Regenerating WAT grammar in thirdparty/tree-sitter-wat"
  SOURCES ${TREE_SITTER_WAT_DIR}/grammar.js
)
