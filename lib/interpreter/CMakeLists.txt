# SPDX-License-Identifier: Apache-2.0

add_library(ssvmInterpreter
  instantiate/import.cpp
  instantiate/function.cpp
  instantiate/global.cpp
  instantiate/table.cpp
  instantiate/memory.cpp
  instantiate/elem.cpp
  instantiate/data.cpp
  instantiate/export.cpp
  instantiate/module.cpp
  engine/proxy.cpp
  engine/control.cpp
  engine/table.cpp
  engine/memory.cpp
  engine/variable.cpp
  engine/engine.cpp
  helper.cpp
  interpreter.cpp
)

target_link_libraries(ssvmInterpreter
  PRIVATE
  ssvmCommon
)

target_include_directories(ssvmInterpreter
  PUBLIC
  ${Boost_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
