# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasiRandomTests
  wasi_random.cpp
)

add_dependencies(wasiRandomTests
  wasmedgePluginWasiRandom
)

target_include_directories(wasiRandomTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasiRandom,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasiRandomTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasiRandomTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasiRandomTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasiRandomTests wasiRandomTests)
