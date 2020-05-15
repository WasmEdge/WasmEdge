# SPDX-License-Identifier: Apache-2.0
add_subdirectory(easyloggingpp)

if(BUILD_TESTS)
  add_subdirectory(googletest)
endif()
