# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasiLoggingTests
  wasi_logging.cpp
)

add_dependencies(wasiLoggingTests
  wasmedgePluginWasiLogging
)

target_include_directories(wasiLoggingTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasiLogging,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasiLoggingTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)

# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasiLoggingTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasiLoggingTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasiLoggingTests wasiLoggingTests)
