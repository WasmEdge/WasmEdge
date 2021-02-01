# SPDX-License-Identifier: Apache-2.0

add_executable(ssvmMemLimitTests
  MemLimitTest.cpp
)

add_test(ssvmMemLimitTests ssvmMemLimitTests)

target_link_libraries(ssvmMemLimitTests
  PRIVATE
  utilGoogleTest
  ssvmVM
)
