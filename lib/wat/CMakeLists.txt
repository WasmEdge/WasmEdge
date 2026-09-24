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
# The runtime and the grammar are private to libwasmedge. Hide their symbols,
# so that an embedder with a tree-sitter of its own does not get a duplicate
# symbol or an allocator hook of ours. The linker script of Linux hides them
# already, but macOS and Windows have no such script. The api.h header of the
# runtime forces the default visibility unless the build defines
# TREE_SITTER_HIDE_SYMBOLS, and parser.c uses the same macro for
# tree_sitter_wat. The alloc.h header of the runtime spells the macro
# TREE_SITTER_HIDDEN_SYMBOLS for the allocator hooks, so both spellings are
# here.
set_target_properties(wasmedgeWatTreeSitter PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED ON
  C_VISIBILITY_PRESET hidden
  POSITION_INDEPENDENT_CODE ON
)
target_compile_definitions(wasmedgeWatTreeSitter
  PRIVATE
  TREE_SITTER_HIDE_SYMBOLS
  TREE_SITTER_HIDDEN_SYMBOLS
)
target_include_directories(wasmedgeWatTreeSitter
  PRIVATE
  ${TREE_SITTER_WAT_DIR}/src
  ${TREE_SITTER_DIR}/include
  ${TREE_SITTER_DIR}/src
)
# This target does not go through wasmedge_setup_target, because the warning
# flags of the project are not for third-party C code. The fuzz build still
# needs the instrumentation, so the fuzzer sees the runtime and the grammar.
if(WASMEDGE_BUILD_FUZZING AND NOT DEFINED LIB_FUZZING_ENGINE)
  target_compile_options(wasmedgeWatTreeSitter
    PRIVATE
    -fsanitize=fuzzer,address
  )
endif()

# Find the locale-aware strtof and strtod of the C library. A float literal of
# the text format must not depend on the LC_NUMERIC locale of the embedder.
# POSIX systems have strtof_l and strtod_l, and Windows has _strtof_l and
# _strtod_l. Some C libraries declare the POSIX functions only with a feature
# macro, so the test tries again with _GNU_SOURCE. float_parse.cpp gets the
# result, and the feature macro does not affect the other files.
include(CheckCXXSourceCompiles)
include(CheckIncludeFileCXX)
include(CMakePushCheckState)
set(WASMEDGE_WAT_FLOAT_DEFINITIONS "")
check_include_file_cxx(xlocale.h WASMEDGE_WAT_HAVE_XLOCALE_H)
if(WASMEDGE_WAT_HAVE_XLOCALE_H)
  list(APPEND WASMEDGE_WAT_FLOAT_DEFINITIONS WASMEDGE_WAT_HAVE_XLOCALE_H)
endif()
set(WASMEDGE_WAT_STRTOD_L_SOURCE [=[
#include <locale.h>
#include <stdlib.h>
#if defined(WASMEDGE_WAT_HAVE_XLOCALE_H)
#include <xlocale.h>
#endif
int main() {
  locale_t Locale = newlocale(LC_ALL_MASK, "C", static_cast<locale_t>(0));
  char *End = nullptr;
  float F = strtof_l("1.5", &End, Locale);
  double D = strtod_l("1.5", &End, Locale);
  freelocale(Locale);
  return static_cast<int>(F + static_cast<float>(D));
}
]=])
set(WASMEDGE_WAT_WIN_STRTOD_L_SOURCE [=[
#include <locale.h>
#include <stdlib.h>
int main() {
  _locale_t Locale = _create_locale(LC_ALL, "C");
  char *End = nullptr;
  float F = _strtof_l("1.5", &End, Locale);
  double D = _strtod_l("1.5", &End, Locale);
  _free_locale(Locale);
  return static_cast<int>(F + static_cast<float>(D));
}
]=])
cmake_push_check_state(RESET)
if(WASMEDGE_WAT_HAVE_XLOCALE_H)
  list(APPEND CMAKE_REQUIRED_DEFINITIONS -DWASMEDGE_WAT_HAVE_XLOCALE_H)
endif()
set(CMAKE_REQUIRED_QUIET ON)
check_cxx_source_compiles("${WASMEDGE_WAT_STRTOD_L_SOURCE}"
  WASMEDGE_WAT_HAVE_STRTOD_L)
if(WASMEDGE_WAT_HAVE_STRTOD_L)
  list(APPEND WASMEDGE_WAT_FLOAT_DEFINITIONS WASMEDGE_WAT_HAVE_STRTOD_L)
else()
  list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
  check_cxx_source_compiles("${WASMEDGE_WAT_STRTOD_L_SOURCE}"
    WASMEDGE_WAT_HAVE_STRTOD_L_WITH_GNU_SOURCE)
  if(WASMEDGE_WAT_HAVE_STRTOD_L_WITH_GNU_SOURCE)
    list(APPEND WASMEDGE_WAT_FLOAT_DEFINITIONS
      _GNU_SOURCE WASMEDGE_WAT_HAVE_STRTOD_L)
  else()
    check_cxx_source_compiles("${WASMEDGE_WAT_WIN_STRTOD_L_SOURCE}"
      WASMEDGE_WAT_HAVE_WIN_STRTOD_L)
    if(WASMEDGE_WAT_HAVE_WIN_STRTOD_L)
      list(APPEND WASMEDGE_WAT_FLOAT_DEFINITIONS WASMEDGE_WAT_HAVE_WIN_STRTOD_L)
    endif()
  endif()
endif()
cmake_pop_check_state()
if(WASMEDGE_WAT_FLOAT_DEFINITIONS MATCHES "HAVE_(WIN_)?STRTOD_L")
  message(STATUS "WAT float parser: ${WASMEDGE_WAT_FLOAT_DEFINITIONS}")
else()
  message(STATUS "WAT float parser: no locale-aware strtod, "
    "the float literals use the LC_NUMERIC locale of the process")
endif()
set_source_files_properties(float_parse.cpp
  PROPERTIES COMPILE_DEFINITIONS "${WASMEDGE_WAT_FLOAT_DEFINITIONS}"
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
  float_parse.cpp
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
