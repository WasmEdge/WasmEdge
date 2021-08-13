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
  engine/control.cpp
  engine/table.cpp
  engine/memory.cpp
  engine/variable.cpp
  engine/engine.cpp
  helper.cpp
  interpreter.cpp
)

target_link_libraries(wasmedgeInterpreter
  PUBLIC
  wasmedgeCommon
  wasmedgeAST
  wasmedgeSystem
)
