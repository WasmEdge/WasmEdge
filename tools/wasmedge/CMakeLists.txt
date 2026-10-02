# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

function(wasmedge_link_tool target)
  if(WASMEDGE_LINK_TOOLS_STATIC)
    target_link_libraries(${target}
      PRIVATE
      wasmedgeCAPI
    )
  else()
    target_link_libraries(${target}
      PRIVATE
      wasmedge_shared
    )
  endif()
endfunction()

if(WASMEDGE_USE_LLVM)
  wasmedge_add_executable(wasmedgec
    wasmedgec.cpp
  )

  install(TARGETS wasmedgec EXPORT wasmedge
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT WasmEdge
  )

  wasmedge_link_tool(wasmedgec)
endif()

wasmedge_add_executable(wasmedge
  wasmedge.cpp
)

install(TARGETS wasmedge EXPORT wasmedge
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT WasmEdge
)

wasmedge_link_tool(wasmedge)


if(WASMEDGE_BUILD_WASI_NN_RPC)
  wasmedge_add_executable(wasi_nn_rpcserver
    wasi_nn_rpcserver.cpp
  )

  target_include_directories(wasi_nn_rpcserver
    SYSTEM BEFORE PUBLIC ${Protobuf_INCLUDE_DIR}
  )

  install(TARGETS wasi_nn_rpcserver EXPORT wasi_nn_rpcserver
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT WasmEdge
  )

  wasmedge_link_tool(wasi_nn_rpcserver)

  target_compile_options(wasi_nn_rpcserver
    PUBLIC
    -DWASMEDGE_BUILD_WASI_NN_RPC
  )
endif()
