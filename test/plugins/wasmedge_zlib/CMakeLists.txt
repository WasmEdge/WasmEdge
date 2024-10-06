# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeZlibTests
  wasmedge_zlib.cpp
)

add_dependencies(wasmedgeZlibTests
  wasmedgePluginWasmEdgeZlib
)

target_include_directories(wasmedgeZlibTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasmEdgeZlib,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgeZlibTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgeZlibTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgeZlibTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgeZlibTests wasmedgeZlibTests)
