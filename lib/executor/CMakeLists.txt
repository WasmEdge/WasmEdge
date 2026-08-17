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
  component/component.cpp
  component/component_executor.cpp
  component/component_alias.cpp
  component/component_canon.cpp
  component/component_canon_abi.cpp
  component/component_lower_thunk.cpp
  component/component_resource_thunk.cpp
  component/component_export.cpp
  component/component_import.cpp
  component/component_instance.cpp
  component/component_module.cpp
  component/component_start.cpp
  component/component_type.cpp
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
