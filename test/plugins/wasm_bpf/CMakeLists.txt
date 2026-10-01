# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeWasmBpfTests
  WasmBpfSimpleMapTest.cpp
  WasmBpfSimpleRingbufTest.cpp
  WasmBpfTest.cpp
)

add_subdirectory(assets)

add_dependencies(wasmedgeWasmBpfTests
  wasmedgePluginWasmBpf
  wasmBpfTestsAssets
)

target_include_directories(wasmedgeWasmBpfTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasmBpf,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgeWasmBpfTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgePluginWasmBpf
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgeWasmBpfTests
    PRIVATE
    wasmedgeCAPI
    wasmedgeExecutor
  )
else()
  target_link_libraries(wasmedgeWasmBpfTests
    PRIVATE
    wasmedge_shared
    wasmedgeExecutor
  )
endif()

add_test(wasmedgeWasmBpfTests wasmedgeWasmBpfTests)
