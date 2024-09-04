# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

# The test plugin module in C API.
enable_language(C)

wasmedge_add_plugin(wasmedgePluginTestModuleC
  SOURCES testplugin.c
)

target_include_directories(wasmedgePluginTestModuleC
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(wasmedgePluginTestModuleC PROPERTIES
  C_STANDARD 11
)

# Remove cxx_standard for MSVC.
set_property(TARGET wasmedgePluginTestModuleC PROPERTY
  CXX_STANDARD
)

# The test plugin module in C++ API.
wasmedge_add_plugin(wasmedgePluginTestModuleCPP
  SOURCES testplugin.cpp
)

target_include_directories(wasmedgePluginTestModuleCPP
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# The test executable for C API
wasmedge_add_executable(wasmedgePluginUnittestsC
  unittest_c.cpp
)

target_include_directories(wasmedgePluginUnittestsC
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgePluginUnittestsC
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)

# The test executable for C++ API
wasmedge_add_executable(wasmedgePluginUnittestsCPP
  unittest_cpp.cpp
)

target_include_directories(wasmedgePluginUnittestsCPP
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgePluginUnittestsCPP
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)

# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginUnittestsC
    PRIVATE
    wasmedgeCAPI
  )
  target_link_libraries(wasmedgePluginUnittestsCPP
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginUnittestsC
    PRIVATE
    wasmedge_shared
  )
  target_link_libraries(wasmedgePluginUnittestsCPP
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgePluginUnittestsC wasmedgePluginUnittestsC)
add_test(wasmedgePluginUnittestsCPP wasmedgePluginUnittestsCPP)
