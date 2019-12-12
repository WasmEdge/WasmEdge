# SPDX-License-Identifier: Apache-2.0

add_executable(ssvmRegressionTests
  main.cpp
)

target_link_libraries(ssvmRegressionTests
  PRIVATE
  utilGoogleTest
  ssvmASTLoad
  ssvmLoader
  ssvmExecutor
)
