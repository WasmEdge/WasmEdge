# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgeHostModuleWasmEdgeProcess
  processfunc.cpp
  processmodule.cpp
)

target_link_libraries(wasmedgeHostModuleWasmEdgeProcess
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
)
