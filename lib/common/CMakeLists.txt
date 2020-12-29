# SPDX-License-Identifier: Apache-2.0

add_library(ssvmCommon
  hexstr.cpp
  log.cpp
  proposal.cpp
)

target_link_libraries(ssvmCommon
  PRIVATE
  utilLog
)

target_include_directories(ssvmCommon
  PUBLIC
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
