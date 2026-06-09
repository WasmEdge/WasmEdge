# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeTensorflowTests
  wasmedge_tensorflow.cpp
)

# On manylinux_2_28 with gcc-toolset and CXX11_ABI=0, LTO can cause unresolved
# internal libstdc++ symbols from libstdc++_nonshared.a (cow-fs_path.o) when
# linking against the TensorFlow C++ shared libraries.
if(CMAKE_COMPILER_IS_GNUCXX AND NOT WASMEDGE_USE_CXX11_ABI)
  set_target_properties(wasmedgeTensorflowTests PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION OFF
  )
endif()

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
