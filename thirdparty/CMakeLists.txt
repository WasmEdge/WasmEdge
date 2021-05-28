# SPDX-License-Identifier: Apache-2.0
add_subdirectory(easyloggingpp)

if (BUILD_AOT_RUNTIME)
  add_subdirectory(blake3)
endif()
