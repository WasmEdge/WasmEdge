# SPDX-License-Identifier: Apache-2.0
add_subdirectory(easyloggingpp)
add_subdirectory(evmc)
add_subdirectory(keccak)

if(BUILD_TESTS)
  add_subdirectory(googletest)
endif()
