# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

set(SOURCES
  compilerTool.cpp
  runtimeTool.cpp
  fuzzTool.cpp
  fuzzPO.cpp
  uniTool.cpp
)

if(WASMEDGE_BUILD_WASI_NN_RPC)
  list(APPEND SOURCES wasiNNRPCServerTool.cpp)
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

if(WASMEDGE_USE_LLVM)
  target_link_libraries(wasmedgeDriver
    PRIVATE
    wasmedgeLoader
    wasmedgeCommon
    wasmedgePO
    wasmedgeVM
    wasmedgeAOT
    wasmedgeLLVM
  )
else()
  target_link_libraries(wasmedgeDriver
    PRIVATE
    wasmedgeLoader
    wasmedgeCommon
    wasmedgePO
    wasmedgeVM
  )
endif()

if(WASMEDGE_USE_LLVM)
  target_compile_definitions(wasmedgeDriver
    PRIVATE
    -DWASMEDGE_USE_LLVM
  )
endif()
