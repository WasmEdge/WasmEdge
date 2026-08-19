# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

# Fetch the tree-sitter runtime (MIT) at configure time. SOURCE_SUBDIR points at
# lib/ (no CMakeLists.txt) so FetchContent populates the source without adding
# tree-sitter's own CMake targets.
include(FetchContent)
# Pin to the commit SHA for tree-sitter v0.26.6 (tags are mutable, SHAs are not)
# for reproducible, supply-chain-safe fetches.
FetchContent_Declare(
  treesitter
  GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
  GIT_TAG        534c4a074cd461ab30d1c8a54bf733d3050221a0 # v0.26.6
  GIT_SHALLOW    TRUE
  SOURCE_SUBDIR  lib
)
FetchContent_MakeAvailable(treesitter)
set(TREE_SITTER_DIR ${treesitter_SOURCE_DIR}/lib)
# Expose the runtime include dir to the spec tests.
set(WASMEDGE_TREE_SITTER_INCLUDE_DIR ${TREE_SITTER_DIR}/include
  CACHE INTERNAL "tree-sitter runtime include directory")

# Generated tree-sitter WAT grammar parser (vendored, first-party Apache-2.0)
set(TREE_SITTER_WAT_DIR ${PROJECT_SOURCE_DIR}/thirdparty/tree-sitter-wat)

# tree-sitter runtime + grammar as a C object library, folded into wasmedgeWat.
add_library(wasmedgeWatTreeSitter OBJECT
  ${TREE_SITTER_DIR}/src/lib.c
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
  ${TREE_SITTER_DIR}/include
  ${TREE_SITTER_DIR}/src
)

# C++ wrapper and parser. The runtime objects are compiled directly in (not
# linked as a separate object library) so the archive is self-contained: every
# consumer -- C API libs, spec tests, wast2wat -- picks up the runtime without
# depending on wasmedgeWatTreeSitter by hand.
wasmedge_add_library(wasmedgeWat
  tree_sitter.cpp
  parser.cpp
  converter.cpp
  convert_types.cpp
  convert_instrs.cpp
  convert_sections.cpp
  maybe_wat.cpp
  $<TARGET_OBJECTS:wasmedgeWatTreeSitter>
)

target_link_libraries(wasmedgeWat
  PUBLIC
  wasmedgeCommon
)

target_include_directories(wasmedgeWat
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${TREE_SITTER_DIR}/include
)

# Grammar regeneration target (developers only).
add_custom_target(wasmedge_generate_wat_grammar
  COMMAND npm install
  COMMAND npx tree-sitter generate grammar.js
  WORKING_DIRECTORY ${TREE_SITTER_WAT_DIR}
  COMMENT "Regenerating WAT grammar in thirdparty/tree-sitter-wat"
  SOURCES ${TREE_SITTER_WAT_DIR}/grammar.js
)
