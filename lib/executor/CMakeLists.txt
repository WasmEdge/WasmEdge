# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_library(wasmedgeExecutor
  instantiate/import.cpp
  instantiate/function.cpp
  instantiate/global.cpp
  instantiate/table.cpp
  instantiate/memory.cpp
  instantiate/elem.cpp
  instantiate/data.cpp
  instantiate/export.cpp
  instantiate/tag.cpp
  instantiate/module.cpp
  engine/proxy.cpp
  engine/controlInstr.cpp
  engine/tableInstr.cpp
  engine/threadInstr.cpp
  engine/memoryInstr.cpp
  engine/variableInstr.cpp
  engine/refInstr.cpp
  engine/engine.cpp
  helper.cpp
  executor.cpp
  coredump.cpp
)

target_link_libraries(wasmedgeExecutor
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
)
