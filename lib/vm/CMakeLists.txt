# SPDX-License-Identifier: Apache-2.0

add_library(wasmedgeVM
  vm.cpp
)

target_link_libraries(wasmedgeVM
  PUBLIC
  wasmedgeCommon
  wasmedgeAST
  wasmedgeLoader
  wasmedgeValidator
  wasmedgeInterpreter
  wasmedgeHostModuleWasi
  wasmedgeHostModuleWasmEdgeProcess
)

target_include_directories(wasmedgeVM
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
