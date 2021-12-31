# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgeLoaderFileMgr
  filemgr.cpp
  ldmgr.cpp
  shared_library.cpp
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
  ast/module.cpp
  ast/section.cpp
  ast/description.cpp
  ast/segment.cpp
  ast/type.cpp
  ast/expression.cpp
  ast/instruction.cpp
  loader.cpp
)

target_link_libraries(wasmedgeLoader
  PUBLIC
  wasmedgeCommon
  wasmedgeLoaderFileMgr
  Boost::boost
  std::filesystem
)
