# SPDX-License-Identifier: Apache-2.0

add_library(ssvmVM
  vm.cpp
)

set(ssvmLibs
  ssvmCommon
  ssvmAST
  ssvmLoader
  ssvmValidator
  ssvmInterpreter
  ssvmHostModuleWasi
  ssvmHostModuleSSVMProcess
)

target_link_libraries(ssvmVM
  PRIVATE
  ${ssvmLibs}
)

target_include_directories(ssvmVM
  PUBLIC
  ${Boost_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
