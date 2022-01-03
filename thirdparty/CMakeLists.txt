# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

if(WASMEDGE_BUILD_AOT_RUNTIME)
  add_subdirectory(blake3)
endif()

set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE)
add_subdirectory(spdlog)
wasmedge_setup_target(spdlog)
