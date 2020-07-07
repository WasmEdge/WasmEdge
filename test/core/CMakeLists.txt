# SPDX-License-Identifier: Apache-2.0

include(FetchContent)
FetchContent_Declare(
  ssvm_unit_test
  GIT_REPOSITORY https://github.com/second-state/ssvm-unittest.git
  GIT_TAG wasm-1.1
)

FetchContent_GetProperties(ssvm_unit_test)
if (NOT ssvm_unit_test_POPULATED)
    FetchContent_Populate(ssvm_unit_test)
endif ()

configure_files(
  ${ssvm_unit_test_SOURCE_DIR}/core
  ${CMAKE_CURRENT_BINARY_DIR}/testSuites
  COPYONLY
)

add_executable(ssvmCoreTests
  coreTest.cpp
)

add_test(ssvmCoreTests ssvmCoreTests)

target_link_libraries(ssvmCoreTests
  PRIVATE
  std::filesystem
  utilGoogleTest
  ssvmVM
)

target_include_directories(ssvmCoreTests  
  PRIVATE
  ${Boost_INCLUDE_DIR}
)
