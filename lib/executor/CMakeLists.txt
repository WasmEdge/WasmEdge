# SPDX-License-Identifier: Apache-2.0

add_subdirectory(instance)
add_subdirectory(instantiate)

add_library(ssvmExecutorWorker
  worker.cpp
  worker/control.cpp
  worker/variable.cpp
  worker/memory.cpp
  worker/provider.cpp
)

target_link_libraries(ssvmExecutorWorker
  PRIVATE
  ssvmExecutorInstance
)

target_include_directories(ssvmExecutorWorker
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty
  ${PROJECT_SOURCE_DIR}/thirdparty/evmc/include
)

add_library(ssvmExecutor
  executor.cpp
)

target_link_libraries(ssvmExecutor
  PRIVATE
  ssvmExecutorInstance
  ssvmExecutorInstantiate
  ssvmExecutorWorker
  ssvmSupport
)

target_include_directories(ssvmExecutor
  PUBLIC
  ${Boost_INCLUDE_DIR}
)