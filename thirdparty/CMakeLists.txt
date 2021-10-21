# SPDX-License-Identifier: Apache-2.0
if(WASMEDGE_BUILD_AOT_RUNTIME)
  add_subdirectory(blake3)
  add_subdirectory(ed25519)
endif()
