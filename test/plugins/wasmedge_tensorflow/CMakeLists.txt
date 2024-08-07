# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeTensorflowTests
  wasmedge_tensorflow.cpp
)

add_dependencies(wasmedgeTensorflowTests
  wasmedgePluginWasmEdgeTensorflow
)

include(WASINNDeps)
wasmedge_setup_tf_target(wasmedgeTensorflowTests)

target_include_directories(wasmedgeTensorflowTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasmEdgeTensorflow,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgeTensorflowTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgeTensorflowTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgeTensorflowTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgeTensorflowTests wasmedgeTensorflowTests)
