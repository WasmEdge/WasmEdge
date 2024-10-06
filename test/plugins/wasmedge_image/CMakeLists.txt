# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeImageTests
  wasmedge_image.cpp
)

add_dependencies(wasmedgeImageTests
  wasmedgePluginWasmEdgeImage
)

target_include_directories(wasmedgeImageTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasmEdgeImage,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgeImageTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgeImageTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgeImageTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgeImageTests wasmedgeImageTests)
