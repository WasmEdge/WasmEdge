# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2026 Second State INC

wasmedge_add_executable(wasmedgeRefLifetimeTests
  RefLifetimeTest.cpp
)

add_test(wasmedgeRefLifetimeTests wasmedgeRefLifetimeTests)

target_link_libraries(wasmedgeRefLifetimeTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeCommon
  Threads::Threads
)

target_include_directories(wasmedgeRefLifetimeTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
