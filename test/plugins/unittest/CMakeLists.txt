# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

# The test plugin module in C API
enable_language(C)

wasmedge_add_library(wasmedgePluginTestModuleC
  SHARED
  testplugin.c
  )

set_target_properties(wasmedgePluginTestModuleC PROPERTIES
  C_STANDARD 11
)

# remove cxx_standard for msvc
set_property(TARGET wasmedgePluginTestModuleC PROPERTY
  CXX_STANDARD
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

target_include_directories(wasmedgePluginTestModuleCPP
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# The test executable for C API
wasmedge_add_executable(wasmedgePluginCTests
  PluginCTest.cpp
)

target_include_directories(wasmedgePluginCTests
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgePluginCTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)

# The test executable for C++ API
wasmedge_add_executable(wasmedgePluginCPPTests
  PluginCPPTest.cpp
)

target_include_directories(wasmedgePluginCPPTests
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgePluginCPPTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
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
  target_link_libraries(wasmedgePluginCTests
    PRIVATE
    wasmedgeCAPI
  )
  target_link_libraries(wasmedgePluginCPPTests
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
  target_link_libraries(wasmedgePluginCTests
    PRIVATE
    wasmedge_shared
  )
  target_link_libraries(wasmedgePluginCPPTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgePluginCTests wasmedgePluginCTests)
add_test(wasmedgePluginCPPTests wasmedgePluginCPPTests)
