# SPDX-License-Identifier: Apache-2.0

add_library(wasmedgeCommon
  hexstr.cpp
  log.cpp
  configure.cpp
)

target_link_libraries(wasmedgeCommon
  PRIVATE
  utilLog
)

target_include_directories(wasmedgeCommon
  PUBLIC
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
