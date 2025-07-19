# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

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
  instantiate/component/component.cpp
  instantiate/component/component_alias.cpp
  instantiate/component/component_canon.cpp
  instantiate/component/component_export.cpp
  instantiate/component/component_import.cpp
  instantiate/component/component_instance.cpp
  instantiate/component/component_start.cpp
  instantiate/component/component_type.cpp
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
