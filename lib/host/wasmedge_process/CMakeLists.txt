# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgeHostModuleWasmEdgeProcess
  processenv.cpp
  processfunc.cpp
  processmodule.cpp
)

target_link_libraries(wasmedgeHostModuleWasmEdgeProcess
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
  wasmedgePlugin
)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_link_options(wasmedgeHostModuleWasmEdgeProcess
    PUBLIC
    -u_ZN8WasmEdge4Host26WasmEdgeProcessEnvironment8RegisterE
  )
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  target_link_options(wasmedgeHostModuleWasmEdgeProcess
    PUBLIC
    -u__ZN8WasmEdge4Host26WasmEdgeProcessEnvironment8RegisterE
  )
endif()
