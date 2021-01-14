# SPDX-License-Identifier: Apache-2.0

add_executable(ssvmProcessTests
  ssvm_process.cpp
)

add_test(ssvmProcessTests ssvmProcessTests)

target_link_libraries(ssvmProcessTests
  PRIVATE
  ssvmAST
  ssvmHostModuleSSVMProcess
  utilGoogleTest
)

target_include_directories(ssvmProcessTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
