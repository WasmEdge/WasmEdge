# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeProcessTests
  wasmedge_process.cpp
)

add_dependencies(wasmedgeProcessTests
  wasmedgePluginWasmEdgeProcess
)

target_include_directories(wasmedgeProcessTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasmEdgeProcess,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgeProcessTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgeProcessTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgeProcessTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgeProcessTests wasmedgeProcessTests)
