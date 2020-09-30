# SPDX-License-Identifier: Apache-2.0

add_executable(ssvmInterpreterCoreTests
  InterpreterTest.cpp
)

add_test(ssvmInterpreterCoreTests ssvmInterpreterCoreTests)

target_link_libraries(ssvmInterpreterCoreTests
  PRIVATE
  std::filesystem
  utilGoogleTest
  ssvmTestSpec
  ssvmVM
)
