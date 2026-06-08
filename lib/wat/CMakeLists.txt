# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2026 Second State INC

# tree-sitter runtime (MIT, v0.26.6). Fetched at configure time like the other
# third-party dependencies (fmt/spdlog/simdjson) rather than vendoring the
# runtime source in-tree. Offline builds can point FetchContent at a local
# checkout via -DFETCHCONTENT_SOURCE_DIR_TREE-SITTER=<path>.
include(FetchContent)
message(STATUS "Downloading tree-sitter source")
FetchContent_Declare(
  tree-sitter
  GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
  GIT_TAG        v0.26.6
  GIT_SHALLOW    TRUE
)
# Populate the source without invoking add_subdirectory: tree-sitter does
# not provide a CMakeLists.txt for its runtime, so we compile the
# amalgamated lib/src/lib.c ourselves below. This mirrors the
# FetchContent_Populate pattern used in cmake/WASINNDeps.cmake for BitNet.
FetchContent_Populate(tree-sitter)
message(STATUS "Downloading tree-sitter source -- done")

# tree-sitter_SOURCE_DIR is only set in this directory scope; export the runtime
# include directory so sibling targets (e.g. test/spec) that include
# <tree_sitter/api.h> can locate it.
set(TREE_SITTER_INCLUDE_DIR ${tree-sitter_SOURCE_DIR}/lib/include
  CACHE INTERNAL "tree-sitter runtime public include directory")

# Generated tree-sitter WAT grammar parser (committed; emitted by tree-sitter-cli)
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
  ${tree-sitter_SOURCE_DIR}/lib/src/lib.c
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
  ${tree-sitter_SOURCE_DIR}/lib/src
  ${tree-sitter_SOURCE_DIR}/lib/include
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
  ${tree-sitter_SOURCE_DIR}/lib/include
)

# Custom target for grammar regeneration (developers only)
add_custom_target(wasmedge_generate_wat_grammar
  COMMAND npm install
  # tree-sitter generate reads grammar.js from the working directory; the CLI
  # (v0.26.x) takes no grammar-file argument.
  COMMAND npx tree-sitter generate
  WORKING_DIRECTORY ${TREE_SITTER_WAT_DIR}
  COMMENT "Regenerating WAT grammar in thirdparty/tree-sitter-wat"
  SOURCES ${TREE_SITTER_WAT_DIR}/grammar.js
)
