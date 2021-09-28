# SPDX-License-Identifier: Apache-2.0

wasmedge_add_library(wasmedgeHostModuleWasiCrypto
  ctx.cpp
)

target_include_directories(wasmedgeHostModuleWasiCrypto
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(wasmedgeHostModuleWasiCrypto
  PUBLIC
  Threads::Threads
  wasmedgeSystem
)

