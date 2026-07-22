# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2025 Second State INC

wasmedge_add_library(wasmedgeHostModuleWasiP2
  wasip2.cpp
)

target_link_libraries(wasmedgeHostModuleWasiP2
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
)

target_include_directories(wasmedgeHostModuleWasiP2
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
)
