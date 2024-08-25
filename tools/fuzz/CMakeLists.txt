# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedge-fuzztool
  tool.cpp
)

wasmedge_add_executable(wasmedge-fuzzpo
  po.cpp
)

if(DEFINED LIB_FUZZING_ENGINE)
  target_link_libraries(wasmedge-fuzztool
    PRIVATE
    ${LIB_FUZZING_ENGINE}
  )
  target_link_libraries(wasmedge-fuzzpo
    PRIVATE
    ${LIB_FUZZING_ENGINE}
  )
endif()

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
