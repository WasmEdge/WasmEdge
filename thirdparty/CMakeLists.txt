# SPDX-License-Identifier: Apache-2.0
add_subdirectory(easyloggingpp)

if (NOT SSVM_DISABLE_AOT_RUNTIME)
  add_subdirectory(blake3)
endif()

if(BUILD_TESTS)
  add_subdirectory(googletest)
endif()
