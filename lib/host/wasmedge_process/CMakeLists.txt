# SPDX-License-Identifier: Apache-2.0

add_library(wasmedgeHostModuleWasmEdgeProcess
  processfunc.cpp
  processmodule.cpp
)

target_link_libraries(wasmedgeHostModuleWasmEdgeProcess
  PRIVATE
  wasmedgeSystem
)

target_include_directories(wasmedgeHostModuleWasmEdgeProcess
  PUBLIC
  ${Boost_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
