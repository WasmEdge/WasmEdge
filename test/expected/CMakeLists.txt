# SPDX-License-Identifier: Apache-2.0

add_executable(expectedTests
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

target_link_libraries(expectedTests
  PRIVATE
  utilGoogleTest
)
