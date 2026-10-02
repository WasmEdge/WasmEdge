# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeOpenCVMiniTests
  WasmEdgeOpenCVMiniTest.cpp
)

# OpenCV headers come in through the plugin's include directories as regular
# includes and trip the shadow warnings; they are not ours to fix.
wasmedge_suppress_shadow_warnings(wasmedgeOpenCVMiniTests)

add_dependencies(wasmedgeOpenCVMiniTests
  wasmedgePluginWasmEdgeOpenCVMini
)

target_include_directories(wasmedgeOpenCVMiniTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasmEdgeOpenCVMini,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgeOpenCVMiniTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgeOpenCVMiniTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgeOpenCVMiniTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgeOpenCVMiniTests wasmedgeOpenCVMiniTests)
