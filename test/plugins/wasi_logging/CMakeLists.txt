# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeWasiLoggingTests
  WasiLoggingTest.cpp
)

add_dependencies(wasmedgeWasiLoggingTests
  wasmedgePluginWasiLogging
)

target_include_directories(wasmedgeWasiLoggingTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasiLogging,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgeWasiLoggingTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)

# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgeWasiLoggingTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgeWasiLoggingTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgeWasiLoggingTests wasmedgeWasiLoggingTests)
