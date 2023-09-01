# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2023 Second State INC

wasmedge_add_executable(wasmedgePluginWasiClocksTests
  wasi_clocks.cpp
)

add_dependencies(wasmedgePluginWasiClocksTests
  wasmedgePluginWasiClocks
)

target_include_directories(wasmedgePluginWasiClocksTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasiClocks,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgePluginWasiClocksTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasiClocksTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasiClocksTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgePluginWasiClocksTests wasmedgePluginWasiClocksTests)
