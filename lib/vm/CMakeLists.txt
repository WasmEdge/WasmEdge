# SPDX-License-Identifier: Apache-2.0

add_library(ssvmVM
  vm.cpp
)

target_link_libraries(ssvmVM
  PRIVATE
  ssvmSupport
  ssvmAST
  ssvmLoader
  ssvmValidator
  ssvmInterpreter
  ssvmHostModuleWasi
  ssvmAOT
)

target_include_directories(ssvmVM
  PUBLIC
  ${Boost_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
