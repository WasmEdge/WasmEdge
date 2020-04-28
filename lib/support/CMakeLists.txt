# SPDX-License-Identifier: Apache-2.0
add_library(ssvmSupport
  log.cpp
)

target_link_libraries(ssvmSupport
  PRIVATE
  utilLog
)