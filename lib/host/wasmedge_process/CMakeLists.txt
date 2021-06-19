# SPDX-License-Identifier: Apache-2.0

add_library(wasmedgeHostModuleWasmEdgeProcess
  processfunc.cpp
  processmodule.cpp
)

target_compile_options(wasmedgeHostModuleWasmEdgeProcess
  PRIVATE
  ${WASMEDGE_CFLAGS}
)

target_link_libraries(wasmedgeHostModuleWasmEdgeProcess
  PUBLIC
  wasmedgeAST
)
