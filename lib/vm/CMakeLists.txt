# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgeVM
  vm.cpp
)

target_link_libraries(wasmedgeVM
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
  wasmedgePlugin
  wasmedgeLoader
  wasmedgeValidator
  wasmedgeExecutor
  wasmedgeHostModuleWasi
)

if(WASMEDGE_USE_LLVM)
  target_compile_definitions(wasmedgeVM
    PUBLIC
    -DWASMEDGE_USE_LLVM
  )
  target_link_libraries(wasmedgeVM
    PUBLIC
    wasmedgeLLVM
  )
endif()
