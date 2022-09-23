# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasmedge-fuzztool
  tool.cpp
)

target_include_directories(wasmedge-fuzztool
  PRIVATE
  ../../include
)

wasmedge_add_executable(wasmedge-fuzzpo
  po.cpp
)

target_include_directories(wasmedge-fuzzpo
  PRIVATE
  ../../include
)

if(WASMEDGE_LINK_TOOLS_STATIC)
  target_link_libraries(wasmedge-fuzztool
    PRIVATE
    wasmedgeCAPI
  )
  target_link_libraries(wasmedge-fuzzpo
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedge-fuzztool
    PRIVATE
    wasmedge_shared
  )
  target_link_libraries(wasmedge-fuzzpo
    PRIVATE
    wasmedge_shared
  )
endif()
