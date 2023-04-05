# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasmBpfTests
  simple_map_test.cpp
  simple_ringbuf_test.cpp
  wasm_bpf.cpp
)

target_link_libraries(wasmBpfTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgePlugin
  wasmedgePluginWasmBpf
  wasmedgeExecutor
)
add_subdirectory(assets)

add_dependencies(
  wasmBpfTests
  wasmBpfTestsAssets
)

add_test(wasmBpfTests wasmBpfTests)
