# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

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
