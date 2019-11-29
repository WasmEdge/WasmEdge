# SPDX-License-Identifier: Apache-2.0

add_subdirectory(hostfunc)

add_library(ssvmVM
  vm.cpp
)

target_link_libraries(ssvmVM
  PRIVATE
  ssvmASTLoad
  ssvmLoader
  ssvmExecutor
  ssvmHostFuncEEI
  ssvmHostFuncWasi
)

if(ONNC_WASM_LIBRARY)
  target_link_libraries(ssvmVM
    PRIVATE
    ssvmHostFuncONNC
  )
endif()
