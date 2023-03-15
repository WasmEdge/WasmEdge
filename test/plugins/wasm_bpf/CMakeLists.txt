# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasmBpfTests
  wasm_bpf.cpp
)

target_link_libraries(wasmBpfTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgePlugin
  wasmedgePluginWasmBpf
)

add_test(wasmBpfTests wasmBpfTests)
