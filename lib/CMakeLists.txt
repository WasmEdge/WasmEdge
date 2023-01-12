# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

if(WASMEDGE_BUILD_AOT_RUNTIME)
  add_subdirectory(aot)
endif()
add_subdirectory(common)
add_subdirectory(system)
add_subdirectory(plugin)
add_subdirectory(po)
add_subdirectory(loader)
add_subdirectory(validator)
add_subdirectory(executor)
add_subdirectory(host)
add_subdirectory(vm)
add_subdirectory(driver)
if(WASMEDGE_BUILD_SHARED_LIB OR WASMEDGE_BUILD_STATIC_LIB OR WASMEDGE_BUILD_TOOLS)
  add_subdirectory(api)
endif()
