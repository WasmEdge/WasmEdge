# SPDX-License-Identifier: Apache-2.0

add_library(utilLog
  easylogging++.cc
)

target_compile_definitions(utilLog
  PUBLIC
  -DELPP_NO_DEFAULT_LOG_FILE
)

target_include_directories(utilLog
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
