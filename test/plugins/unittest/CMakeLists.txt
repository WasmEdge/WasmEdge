# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

# The test plugin module in C API
wasmedge_add_library(wasmedgePluginTestModuleC
  SHARED
  testplugin.c
)

set_target_properties(wasmedgePluginTestModuleC PROPERTIES
  C_STANDARD 11
)

target_compile_options(wasmedgePluginTestModuleC
  PUBLIC
  -DWASMEDGE_PLUGIN
)

# The test plugin module in C++ API
wasmedge_add_library(wasmedgePluginTestModuleCPP
  SHARED
  testplugin.cpp
)

target_compile_options(wasmedgePluginTestModuleCPP
  PUBLIC
  -DWASMEDGE_PLUGIN
)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  target_link_options(wasmedgePluginTestModuleCPP
    PUBLIC
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterC1EPKNS0_6Plugin16PluginDescriptorE
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterD1Ev
  )
endif()

target_include_directories(wasmedgePluginTestModuleCPP
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# The test executable for C API
wasmedge_add_executable(wasmedgePluginUnittestsC
  unittest_c.cpp
)

target_include_directories(wasmedgePluginUnittestsC
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgePluginUnittestsC
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgePlugin
)

# The test executable for C++ API
wasmedge_add_executable(wasmedgePluginUnittestsCPP
  unittest_cpp.cpp
)

target_include_directories(wasmedgePluginUnittestsCPP
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgePluginUnittestsCPP
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgePlugin
)

# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginTestModuleC
    PRIVATE
    wasmedgeCAPI
  )
  target_link_libraries(wasmedgePluginTestModuleCPP
    PRIVATE
    wasmedgeCAPI
  )
  target_link_libraries(wasmedgePluginUnittestsC
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginTestModuleC
    PRIVATE
    wasmedge_shared
  )
  target_link_libraries(wasmedgePluginTestModuleCPP
    PRIVATE
    wasmedge_shared
  )
  target_link_libraries(wasmedgePluginUnittestsC
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgePluginUnittestsC wasmedgePluginUnittestsC)
add_test(wasmedgePluginUnittestsCPP wasmedgePluginUnittestsCPP)
