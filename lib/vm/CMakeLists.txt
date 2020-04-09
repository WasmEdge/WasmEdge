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
  ssvmHostModuleEEI
  ssvmHostModuleWasi
  ssvmAOT
)

if(ONNC_WASM_LIBRARY)
  target_link_libraries(ssvmVM
    PRIVATE
    ssvmHostModuleONNC
  )
endif()

target_include_directories(ssvmVM
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
  ${PROJECT_SOURCE_DIR}/thirdparty/evmc/include
)
