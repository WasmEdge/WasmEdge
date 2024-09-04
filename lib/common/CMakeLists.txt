# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_setup_spdlog()

wasmedge_add_library(wasmedgeCommon
  hexstr.cpp
  spdlog.cpp
  errinfo.cpp
)

target_link_libraries(wasmedgeCommon
  PUBLIC
  spdlog::spdlog
)

target_include_directories(wasmedgeCommon
  PUBLIC
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_SOURCE_DIR}/include
)
