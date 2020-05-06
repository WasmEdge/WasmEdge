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

target_include_directories(ssvmExecutorInstantiate
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
  ${PROJECT_SOURCE_DIR}/thirdparty/evmc/include
)
