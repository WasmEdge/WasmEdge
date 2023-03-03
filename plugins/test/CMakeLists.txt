# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgePluginTest
  SHARED
  test.c
)

set_target_properties(wasmedgePluginTest PROPERTIES
  C_STANDARD 11
)

target_compile_options(wasmedgePluginTest
  PUBLIC
  -DWASMEDGE_PLUGIN
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginTest
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginTest
    PRIVATE
    wasmedge_shared
  )
endif()
