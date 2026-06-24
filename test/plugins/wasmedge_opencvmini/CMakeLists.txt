# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeOpencvminiTests
  wasmedge_opencvmini.cpp
)

# OpenCV headers come in through the plugin's include directories as regular
# includes and trip the shadow warnings; they are not ours to fix.
wasmedge_suppress_shadow_warnings(wasmedgeOpencvminiTests)

add_dependencies(wasmedgeOpencvminiTests
  wasmedgePluginWasmEdgeOpenCVMini
)

target_include_directories(wasmedgeOpencvminiTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasmEdgeOpenCVMini,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgeOpencvminiTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgeOpencvminiTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgeOpencvminiTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgeOpencvminiTests wasmedgeOpencvminiTests)
