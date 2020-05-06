# SPDX-License-Identifier: Apache-2.0

add_subdirectory(hostfunc)

add_library(ssvmVM
  environment.cpp
  vm.cpp
)

target_link_libraries(ssvmVM
  PRIVATE
  ssvmSupport
  ssvmAST
  ssvmLoader
  ssvmValidator
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

target_include_directories(ssvmVM
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
  ${PROJECT_SOURCE_DIR}/thirdparty/evmc/include
  ${Boost_INCLUDE_DIR}
)
