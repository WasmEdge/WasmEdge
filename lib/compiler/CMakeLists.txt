# SPDX-License-Identifier: Apache-2.0

find_package(LLVM REQUIRED HINTS "${LLVM_CMAKE_PATH}")
list(APPEND CMAKE_MODULE_PATH ${LLVM_DIR})
include(AddLLVM)
include_directories(SYSTEM "${LLVM_INCLUDE_DIR}")

add_library(ssvmCompiler
  compiler.cpp
  library.cpp
)

llvm_map_components_to_libnames(llvm_libs
  core
  executionengine
  native
  nativecodegen
  orcjit
  passes
  transformutils
)

target_link_libraries(ssvmCompiler
  PUBLIC
  ${llvm_libs}
  PRIVATE
  ssvmLoader
  ssvmCompilerHostFuncEEI
  ssvmCompilerHostFuncWasi
  ssvmVM
)

add_subdirectory(hostfunc)
