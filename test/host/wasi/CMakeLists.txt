# SPDX-License-Identifier: Apache-2.0

add_executable(wasiTests
  wasi.cpp
)

add_test(wasiTests wasiTests)

target_link_libraries(wasiTests
  PRIVATE
  ssvmAST
  ssvmHostModuleWasi
  utilGoogleTest
)

target_include_directories(wasiTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
