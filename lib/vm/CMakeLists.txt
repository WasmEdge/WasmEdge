# SPDX-License-Identifier: Apache-2.0

add_library(ssvmVM
  vm.cpp
)

set(ssvmLibs
  ssvmSupport
  ssvmAST
  ssvmLoader
  ssvmValidator
  ssvmInterpreter
  ssvmHostModuleWasi
)

if (NOT SSVM_DISABLE_AOT_RUNTIME)
  set(ssvmLibs
    ${ssvmLibs}
    ssvmAOT
  )
endif()

target_link_libraries(ssvmVM
  PRIVATE
  ${ssvmLibs}
)

target_include_directories(ssvmVM
  PUBLIC
  ${Boost_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
