# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

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
  ast/component/sort.cpp
  ast/component/instance.cpp
  ast/component/alias.cpp
  ast/component/type.cpp
  ast/component/canonical.cpp
  ast/component/start.cpp
  ast/component/import_export.cpp
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
