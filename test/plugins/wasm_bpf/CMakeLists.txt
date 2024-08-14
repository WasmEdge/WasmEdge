# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmBpfTests
  simple_map_test.cpp
  simple_ringbuf_test.cpp
  wasm_bpf.cpp
)

add_subdirectory(assets)

add_dependencies(wasmBpfTests
  wasmedgePluginWasmBpf
  wasmBpfTestsAssets
)

target_include_directories(wasmBpfTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasmBpf,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmBpfTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgePluginWasmBpf
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmBpfTests
    PRIVATE
    wasmedgeCAPI
    wasmedgeExecutor
  )
else()
  target_link_libraries(wasmBpfTests
    PRIVATE
    wasmedge_shared
    wasmedgeExecutor
  )
endif()

add_test(wasmBpfTests wasmBpfTests)
