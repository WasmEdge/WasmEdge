# SPDX-License-Identifier: Apache-2.0

add_subdirectory(instantiate)
add_subdirectory(engine)

add_library(ssvmInterpreter
  interpreter.cpp
)

target_link_libraries(ssvmInterpreter
  PRIVATE
  ssvmInterpreterInstantiate
  ssvmInterpreterEngine
  ssvmSupport
)

target_include_directories(ssvmInterpreter
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
