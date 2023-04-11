# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

# The test plugin module
wasmedge_add_library(wasmedgePluginTestModule
  SHARED
  testplugin.cpp
)

target_compile_options(wasmedgePluginTestModule
  PUBLIC
  -DWASMEDGE_PLUGIN
)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  target_link_options(wasmedgePluginTestModule
    PUBLIC
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterC1EPKNS0_6Plugin16PluginDescriptorE
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterD1Ev
  )
endif()

target_include_directories(wasmedgePluginTestModule
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginTestModule
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginTestModule
    PRIVATE
    wasmedge_shared
  )
endif()

# The test executable
wasmedge_add_executable(wasmedgePluginUnittests
  unittest.cpp
)

target_include_directories(wasmedgePluginUnittests
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgePluginUnittests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgePlugin
)

add_test(wasmedgePluginUnittests wasmedgePluginUnittests)
