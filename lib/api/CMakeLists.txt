# SPDX-License-Identifier: Apache-2.0

if(WASMEDGE_DISABLE_AOT_RUNTIME)
  add_definitions(-DWASMEDGE_DISABLE_AOT_RUNTIME)
endif()

add_library(wasmedge_c SHARED
  wasmedge.cpp
)

target_link_libraries(wasmedge_c
  PUBLIC
  wasmedgeVM
)

if (NOT WASMEDGE_DISABLE_AOT_RUNTIME)
  target_link_libraries(wasmedge_c
    PUBLIC
    wasmedgeAOT
  )
endif()

target_include_directories(wasmedge_c
  PUBLIC
  ${PROJECT_BINARY_DIR}/include
)
