# SPDX-License-Identifier: Apache-2.0

add_library(ssvmExecutorInstantiate
  import.cpp
  type.cpp
  function.cpp
  global.cpp
  table.cpp
  memory.cpp
  export.cpp
  module.cpp
)

target_link_libraries(ssvmExecutorInstantiate
  PRIVATE
  ssvmExecutorInstance
)
