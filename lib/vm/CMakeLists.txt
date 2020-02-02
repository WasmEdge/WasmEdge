# SPDX-License-Identifier: Apache-2.0

add_subdirectory(hostfunc)

add_library(ssvmVM
  environment.cpp
  vm.cpp
)

target_link_libraries(ssvmVM
  PRIVATE
  ssvmSupport
  ssvmASTLoad
  ssvmLoader
  ssvmValidator
  ssvmExecutor
  ssvmHostFuncEEI
  ssvmHostFuncWasi
  utilLog
)

if(ONNC_WASM_LIBRARY)
  target_link_libraries(ssvmVM
    PRIVATE
    ssvmHostFuncONNC
  )
endif()
