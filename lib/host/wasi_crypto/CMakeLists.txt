# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

find_package(OpenSSL REQUIRED)

wasmedge_add_library(wasmedgeHostModuleWasiCrypto
  module.cpp
  common/func.cpp
  symmetric/func.cpp
  asymmetric_common/func.cpp
  kx/func.cpp
  signatures/func.cpp
  common/ctx.cpp
  common/array_output.cpp
)

target_include_directories(wasmedgeHostModuleWasiCrypto
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(wasmedgeHostModuleWasiCrypto
  PUBLIC
  wasmedgeSystem
  OpenSSL::Crypto
)
