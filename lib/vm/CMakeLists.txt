# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgeVM
  vm.cpp
)

target_link_libraries(wasmedgeVM
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
  wasmedgeLoader
  wasmedgeValidator
  wasmedgeExecutor
  wasmedgeHostModuleWasi
  wasmedgeHostModuleWasmEdgeProcess
)

if(WASMEDGE_BUILD_WASI_CRYPTO) 
  target_link_libraries(wasmedgeVM
    PUBLIC
    wasmedgeHostModuleWasiCrypto
  )
endif()
