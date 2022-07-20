# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

if(WASMEDGE_BUILD_TOOLS AND WASMEDGE_BUILD_SHARED_LIB)
  if(WASMEDGE_BUILD_AOT_RUNTIME)
    wasmedge_add_executable(wasmedgec
      wasmedgec.cpp
    )

    install(TARGETS wasmedgec EXPORT wasmedge
      DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    target_link_libraries(wasmedgec
      PRIVATE
      wasmedge_c_shared
    )
  endif()

  wasmedge_add_executable(wasmedge
    wasmedger.cpp
  )

  install(TARGETS wasmedge EXPORT wasmedge
    DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  target_link_libraries(wasmedge
    PRIVATE
    wasmedge_c_shared
  )
endif()

if(WASMEDGE_BUILD_STATIC_TOOLS AND WASMEDGE_BUILD_STATIC_LIB)
  if(WASMEDGE_BUILD_AOT_RUNTIME)
    wasmedge_add_executable(wasmedgec_static
      wasmedgec.cpp
    )

    install(TARGETS wasmedgec EXPORT wasmedge
      DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    target_link_libraries(wasmedgec_static
      PRIVATE
      wasmedge_c_static
    )
  endif()

  wasmedge_add_executable(wasmedge_static
    wasmedger.cpp
  )

  install(TARGETS wasmedge EXPORT wasmedge
    DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  target_link_libraries(wasmedge_static
    PRIVATE
    wasmedge_c_static
  )
endif()
