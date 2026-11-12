# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_library(wasmedgeVM
  plugin_modules.cpp
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

if(MSVC)
  target_compile_options(wasmedgeVM PRIVATE "/bigobj")
endif()
