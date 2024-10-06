# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

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
  target_link_libraries(wasmedgePluginUnittestsCPP
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
  target_link_libraries(wasmedgePluginUnittestsCPP
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgePluginUnittestsC wasmedgePluginUnittestsC)
add_test(wasmedgePluginUnittestsCPP wasmedgePluginUnittestsCPP)
