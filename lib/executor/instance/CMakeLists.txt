# SPDX-License-Identifier: Apache-2.0

add_library(ssvmExecutorInstance
  entity.cpp
  function.cpp
  global.cpp
  memory.cpp
  module.cpp
  table.cpp
)

target_include_directories(ssvmExecutorInstance
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
