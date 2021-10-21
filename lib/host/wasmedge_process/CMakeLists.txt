# SPDX-License-Identifier: Apache-2.0

wasmedge_add_library(wasmedgeHostModuleWasmEdgeProcess
  processfunc.cpp
  processmodule.cpp
)

target_link_libraries(wasmedgeHostModuleWasmEdgeProcess
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
)
