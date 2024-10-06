# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgeAOT
  blake3.cpp
  cache.cpp
)

target_link_libraries(wasmedgeAOT
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
  utilBlake3
  std::filesystem
)

target_include_directories(wasmedgeAOT
  SYSTEM
  PRIVATE
  ${LLVM_INCLUDE_DIR}
)

target_include_directories(wasmedgeAOT
  PUBLIC
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty/blake3
)
