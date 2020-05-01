# SPDX-License-Identifier: Apache-2.0

add_executable(spanTests
  span.cpp
)

target_link_libraries(spanTests
  PRIVATE
  utilGoogleTest
)

target_include_directories(spanTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
