# SPDX-License-Identifier: Apache-2.0

add_library(wasmedgeHostModuleWasmEdgeProcess
  processfunc.cpp
  processmodule.cpp
)

target_link_libraries(wasmedgeHostModuleWasmEdgeProcess
  PUBLIC
  wasmedgeAST
)

target_include_directories(wasmedgeHostModuleWasmEdgeProcess
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
