# SPDX-License-Identifier: Apache-2.0

add_library(ssvmValidator
  formchecker.cpp
  validator.cpp
)

target_link_libraries(ssvmValidator
  PRIVATE
  ssvmCommon
)

target_include_directories(ssvmValidator
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
