# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

if(WASMEDGE_USE_LLVM)
  wasmedge_add_executable(wasmedgec
    wasmedgec.cpp
  )

  install(TARGETS wasmedgec EXPORT wasmedge
    DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  if(WASMEDGE_LINK_TOOLS_STATIC)
    target_link_libraries(wasmedgec
      PRIVATE
      wasmedgeCAPI
    )
  else()
    target_link_libraries(wasmedgec
      PRIVATE
      wasmedge_shared
    )
  endif()
endif()

wasmedge_add_executable(wasmedge
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


if(WASMEDGE_BUILD_WASI_NN_RPC)
  wasmedge_add_executable(wasi_nn_rpcserver
    wasi_nn_rpcserver.cpp
  )

  target_include_directories(wasi_nn_rpcserver
    SYSTEM BEFORE PUBLIC ${Protobuf_INCLUDE_DIR}
  )

  install(TARGETS wasi_nn_rpcserver EXPORT wasi_nn_rpcserver
    DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  if(WASMEDGE_LINK_TOOLS_STATIC)
    target_link_libraries(wasi_nn_rpcserver
      PRIVATE
      wasmedgeCAPI
    )
  else()
    target_link_libraries(wasi_nn_rpcserver
      PRIVATE
      wasmedge_shared
    )
  endif()

  target_compile_options(wasi_nn_rpcserver
    PUBLIC
    -DWASMEDGE_BUILD_WASI_NN_RPC
  )
endif()
