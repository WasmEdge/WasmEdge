# SPDX-License-Identifier: Apache-2.0

add_executable(ssvmRegressionTests
  regression.cpp
  main.cpp
)

target_link_libraries(ssvmRegressionTests
  PRIVATE
  utilGoogleTest
  ssvmASTLoad
  ssvmLoader
  ssvmExecutor
)
