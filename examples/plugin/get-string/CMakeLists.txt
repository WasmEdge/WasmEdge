# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

add_library(GetString
  SHARED
  getstring.cpp
)

target_compile_options(GetString
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(GetString
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(GetString
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(GetString
    PRIVATE
    wasmedge_shared
  )
endif()
