# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

if(WASMEDGE_BUILD_AOT_RUNTIME)
  wasmedge_add_executable(wasmedgec
    wasmedgec.cpp
  )

  install(TARGETS wasmedgec EXPORT wasmedge
    DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  if(WASMEDGE_BUILD_TOOLS_STATIC)
    target_link_libraries(wasmedgec
      PRIVATE
      wasmedge_c_static
      PUBLIC
      dl
      pthread
    )
    if(APPLE)
      target_link_libraries(wasmedgec
        PUBLIC
        ncurses
        z
        xar
      )
    elseif(UNIX)
      target_link_libraries(wasmedgec
        PUBLIC
        rt
      )
    endif()
  else()
    target_link_libraries(wasmedgec
      PRIVATE
      wasmedge_c_shared
    )
  endif()
endif()

wasmedge_add_executable(wasmedge
  wasmedger.cpp
)

install(TARGETS wasmedge EXPORT wasmedge
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WASMEDGE_BUILD_TOOLS_STATIC)
  target_link_libraries(wasmedge
    PRIVATE
    wasmedge_c_static
    PUBLIC
    dl
    pthread
  )
  if(APPLE)
    target_link_libraries(wasmedge
      PUBLIC
      ncurses
      z
      xar
    )
  elseif(UNIX)
    target_link_libraries(wasmedge
      PUBLIC
      rt
    )
  endif()
else()
  target_link_libraries(wasmedge
    PRIVATE
    wasmedge_c_shared
  )
endif()
