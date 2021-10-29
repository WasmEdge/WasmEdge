# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgeVM
  vm.cpp
)

if(WASMEDGE_WASINN_BUILD_OPENVINO)
  add_definitions(-DWASMEDGE_WASINN_BUILD_OPENVINO)
endif()

target_link_libraries(wasmedgeVM
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
  wasmedgePlugin
  wasmedgeLoader
  wasmedgeValidator
  wasmedgeExecutor
  wasmedgeSignature
  wasmedgeHostModuleWasi
)
