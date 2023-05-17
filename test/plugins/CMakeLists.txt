# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

if(WASMEDGE_PLUGIN_PROCESS)
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_subdirectory(wasmedge_process)
  endif()
endif()

if(WASMEDGE_PLUGIN_WASI_NN_BACKEND)
  add_subdirectory(wasi_nn)
endif()

if(WASMEDGE_PLUGIN_HTTPSREQ)
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_subdirectory(wasmedge_httpsreq)
  endif()
endif()

if(WASMEDGE_PLUGIN_WASI_CRYPTO)
  add_subdirectory(wasi_crypto)
endif()

if(WASMEDGE_PLUGIN_WASM_BPF)
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_subdirectory(wasm_bpf)
  endif()
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "Darwin")
  add_subdirectory(unittest)
endif()
