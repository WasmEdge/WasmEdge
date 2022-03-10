# SPDX-License-Identifier: Apache-2.0

wasmedge_add_library(wasmedgeHostModuleWasiNN
  wasinnfunc.cpp
  wasinnmodule.cpp
)

target_link_libraries(wasmedgeHostModuleWasiNN
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
)
