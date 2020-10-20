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
  ssvmCommon
)

target_include_directories(ssvmInterpreter
  PUBLIC
  ${Boost_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
