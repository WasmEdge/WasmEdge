# SPDX-License-Identifier: Apache-2.0

add_library(ssvmExpVM
  vm.cpp
)

target_link_libraries(ssvmExpVM
  PRIVATE
  ssvmSupport
  ssvmAST
  ssvmLoader
  ssvmValidator
  ssvmInterpreter
  ssvmHostModuleEEI
  ssvmHostModuleWasi
)

if(ONNC_WASM_LIBRARY)
  target_link_libraries(ssvmExpVM
    PRIVATE
    ssvmHostModuleONNC
  )
endif()

target_include_directories(ssvmExpVM
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
  ${PROJECT_SOURCE_DIR}/thirdparty/evmc/include
)
