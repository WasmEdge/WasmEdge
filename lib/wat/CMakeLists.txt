# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

# Fetch the tree-sitter runtime, which has the MIT license, at configure time.
# SOURCE_SUBDIR points at lib/, and lib/ has no CMakeLists.txt. FetchContent
# then gets the source without the CMake targets of tree-sitter.
include(FetchContent)
# Pin the fetch to the commit SHA of tree-sitter v0.26.6. A tag can change, but
# a SHA cannot change. The fetch is then reproducible and safe for the supply
# chain.
FetchContent_Declare(
  treesitter
  GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
  GIT_TAG        534c4a074cd461ab30d1c8a54bf733d3050221a0 # v0.26.6
  GIT_SHALLOW    TRUE
  SOURCE_SUBDIR  lib
)
FetchContent_MakeAvailable(treesitter)
set(TREE_SITTER_DIR ${treesitter_SOURCE_DIR}/lib)
# Give the include directory of the runtime to the spec tests.
set(WASMEDGE_TREE_SITTER_INCLUDE_DIR ${TREE_SITTER_DIR}/include
  CACHE INTERNAL "tree-sitter runtime include directory")

# The tree-sitter parser of the WAT grammar. The generator made these files.
# They are vendored, they are first-party, and they have the Apache-2.0
# license.
set(TREE_SITTER_WAT_DIR ${PROJECT_SOURCE_DIR}/thirdparty/tree-sitter-wat)

# Build the tree-sitter runtime and the grammar as a C object library.
# wasmedgeWat then holds these objects.
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

# The C++ wrapper and the parser. This target compiles the runtime objects
# directly, and it does not link them as a separate object library. The archive
# is then complete on its own. Every consumer gets the runtime without a
# dependency on wasmedgeWatTreeSitter by hand. The consumers are the C API
# libraries, the spec tests, and wast2wat.
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

# This target makes the grammar files again. It is only for a developer.
add_custom_target(wasmedge_generate_wat_grammar
  COMMAND npm install
  COMMAND npx tree-sitter generate grammar.js
  WORKING_DIRECTORY ${TREE_SITTER_WAT_DIR}
  COMMENT "Regenerating WAT grammar in thirdparty/tree-sitter-wat"
  SOURCES ${TREE_SITTER_WAT_DIR}/grammar.js
)
