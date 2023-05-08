# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasmedgeHttpsReqTests
  httpsreq.cpp
)

add_dependencies(wasmedgeHttpsReqTests
  wasmedgePluginHttpsReq
)

target_include_directories(wasmedgeHttpsReqTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginHttpsReq,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgeHttpsReqTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgeHttpsReqTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgeHttpsReqTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgeHttpsReqTests wasmedgeHttpsReqTests)
