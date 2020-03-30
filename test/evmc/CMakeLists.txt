# SPDX-License-Identifier: Apache-2.0

add_executable(ssvmEVMCTests
  evmcTest.cpp
  example_host.cpp
)

target_link_libraries(ssvmEVMCTests
  PRIVATE
  utilEVMCLoader
  utilGoogleTest
  ${CMAKE_DL_LIBS}
)
