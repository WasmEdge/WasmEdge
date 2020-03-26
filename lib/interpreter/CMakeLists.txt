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
)