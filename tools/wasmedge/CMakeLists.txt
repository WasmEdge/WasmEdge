# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasmedge
  wincli_helper.cpp
  wasmedge.cpp
)

install(TARGETS wasmedge EXPORT wasmedge
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WASMEDGE_LINK_TOOLS_STATIC)
  target_link_libraries(wasmedge
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedge
    PRIVATE
    wasmedge_shared
  )
endif()

