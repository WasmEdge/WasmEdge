# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasmEdgeRUSTLSTests
  wasmedge_rustls.cpp
)

add_dependencies(wasmEdgeRUSTLSTests
  wasmedge_rustls
)

target_include_directories(wasmEdgeRUSTLSTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedge_rustls,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmEdgeRUSTLSTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmEdgeRUSTLSTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmEdgeRUSTLSTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmEdgeRUSTLSTests wasmEdgeRUSTLSTests)
