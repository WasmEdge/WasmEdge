# SPDX-License-Identifier: Apache-2.0

wasmedge_add_library(wasmedgeInterpreter
  instantiate/import.cpp
  instantiate/function.cpp
  instantiate/global.cpp
  instantiate/table.cpp
  instantiate/memory.cpp
  instantiate/elem.cpp
  instantiate/data.cpp
  instantiate/export.cpp
  instantiate/module.cpp
  engine/proxy.cpp
  engine/controlInstr.cpp
  engine/tableInstr.cpp
  engine/memoryInstr.cpp
  engine/variableInstr.cpp
  engine/engine.cpp
  helper.cpp
  interpreter.cpp
)

target_link_libraries(wasmedgeInterpreter
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
)
