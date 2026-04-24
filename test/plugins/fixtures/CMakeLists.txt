# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

# Incomplete plugin (missing symbols)
wasmedge_add_library(wasmedgePluginTestIncomplete
  SHARED
  incomplete_plugin.c
)

set_target_properties(wasmedgePluginTestIncomplete PROPERTIES
  C_STANDARD 11
)

target_compile_options(wasmedgePluginTestIncomplete
  PUBLIC
  -DWASMEDGE_PLUGIN
)

# Version mismatch plugin
wasmedge_add_library(wasmedgePluginTestVersionMismatch
  SHARED
  version_mismatch_plugin.c
)

set_target_properties(wasmedgePluginTestVersionMismatch PROPERTIES
  C_STANDARD 11
)

target_compile_options(wasmedgePluginTestVersionMismatch
  PUBLIC
  -DWASMEDGE_PLUGIN
)

# Link to WasmEdge
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginTestIncomplete
    PRIVATE
    wasmedgeCAPI
  )
  target_link_libraries(wasmedgePluginTestVersionMismatch
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginTestIncomplete
    PRIVATE
    wasmedge_shared
  )
  target_link_libraries(wasmedgePluginTestVersionMismatch
    PRIVATE
    wasmedge_shared
  )
endif()
