# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasiSocketTests
  wasi_socket.cpp
)

add_test(wasiSocketTests wasiSocketTests)

target_link_libraries(wasiSocketTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeHostModuleWasi
  wasmedgeExecutor
  wasmedgeLoader
)

if(WIN32)
  target_link_libraries(wasiSocketTests
  PRIVATE
  wsock32
  ws2_32
)
endif()
