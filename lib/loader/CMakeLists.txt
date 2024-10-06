# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgeLoaderFileMgr
  filemgr.cpp
  shared_library.cpp
  aot_section.cpp
)

target_link_libraries(wasmedgeLoaderFileMgr
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
  std::filesystem
)

if(NOT WIN32)
  target_link_libraries(wasmedgeLoaderFileMgr
    PUBLIC
    dl
  )
endif()

wasmedge_add_library(wasmedgeLoader
  ast/component.cpp
  ast/component/component_alias.cpp
  ast/component/component_canonical.cpp
  ast/component/component_import_export.cpp
  ast/component/component_instance.cpp
  ast/component/component_start.cpp
  ast/component/component_sort.cpp
  ast/component/component_type.cpp
  ast/module.cpp
  ast/section.cpp
  ast/description.cpp
  ast/segment.cpp
  ast/type.cpp
  ast/expression.cpp
  ast/instruction.cpp
  serialize/serial_description.cpp
  serialize/serial_expression.cpp
  serialize/serial_instruction.cpp
  serialize/serial_module.cpp
  serialize/serial_section.cpp
  serialize/serial_segment.cpp
  serialize/serial_type.cpp
  loader.cpp
)

target_link_libraries(wasmedgeLoader
  PUBLIC
  wasmedgeCommon
  wasmedgeLoaderFileMgr
  std::filesystem
)
