# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeTensorflowLiteTests
  wasmedge_tensorflowlite.cpp
)

add_dependencies(wasmedgeTensorflowLiteTests
  wasmedgePluginWasmEdgeTensorflowLite
)

include(WASINNDeps)
wasmedge_setup_tflite_target(wasmedgeTensorflowLiteTests)

target_include_directories(wasmedgeTensorflowLiteTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasmEdgeTensorflowLite,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgeTensorflowLiteTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgeTensorflowLiteTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgeTensorflowLiteTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgeTensorflowLiteTests wasmedgeTensorflowLiteTests)
