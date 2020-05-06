# SPDX-License-Identifier: Apache-2.0
add_library(ssvmSupport
  log.cpp
)

target_link_libraries(ssvmSupport
  PRIVATE
  utilLog
)

target_include_directories(ssvmSupport
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
