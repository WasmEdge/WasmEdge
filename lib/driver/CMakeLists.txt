# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

set(SOURCES
  compilerTool.cpp
  runtimeTool.cpp
  fuzzTool.cpp
  fuzzPO.cpp
  uniTool.cpp
  validateTool.cpp
  parseTool.cpp
  instantiateTool.cpp
  toolConfig.cpp
)

if(WASMEDGE_BUILD_WASI_NN_RPC)
  list(APPEND SOURCES wasiNNRPCServerTool.cpp)
endif()

if(WASMEDGE_BUILD_SIGNATURE_TOOLS)
  list(APPEND SOURCES signTool.cpp verifyTool.cpp signature_crypto.cpp)
endif()

wasmedge_add_library(wasmedgeDriver
  ${SOURCES}
)

if(WASMEDGE_BUILD_WASI_NN_RPC)
  target_link_libraries(wasmedgeDriver
    PRIVATE
    wasiNNRPC
  )
endif()

target_link_libraries(wasmedgeDriver
  PUBLIC
  wasmedgeVM
)

if(WASMEDGE_BUILD_SIGNATURE_TOOLS)
  target_link_libraries(wasmedgeDriver
    PRIVATE
    OpenSSL::Crypto
  )
  target_compile_definitions(wasmedgeDriver
    PRIVATE
    -DWASMEDGE_BUILD_SIGNATURE_TOOLS
  )
endif()

if(WASMEDGE_USE_LLVM)
  target_compile_definitions(wasmedgeDriver
    PRIVATE
    -DWASMEDGE_USE_LLVM
  )
endif()
