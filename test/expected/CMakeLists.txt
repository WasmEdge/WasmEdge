# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(expectedTests
  assignment.cpp
  bases.cpp
  constexpr.cpp
  constructors.cpp
  emplace.cpp
  extensions.cpp
  gtest.cpp
  issues.cpp
  noexcept.cpp
  observers.cpp
  relops.cpp
  swap.cpp
)

add_test(expectedTests expectedTests)

target_link_libraries(expectedTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)

target_include_directories(expectedTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
