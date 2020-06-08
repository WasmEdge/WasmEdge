# SPDX-License-Identifier: Apache-2.0

add_subdirectory(ssvm)
if (NOT SSVM_DISABLE_AOT_RUNTIME)
  add_subdirectory(ssvm-aot)
endif()
