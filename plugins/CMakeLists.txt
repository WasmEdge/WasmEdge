# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

add_subdirectory(wasmedge_process)
add_subdirectory(wasmedge_thread)
if(WASMEDGE_WASINN_BACKEND)
  add_subdirectory(wasi_nn)
endif()

if(WASMEDGE_PLUGIN_HTTPSREQ) 
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_subdirectory(wasmedge_httpsreq)
  endif()
endif()

if (WASMEDGE_PLUGIN_WASI_CRYPTO)
  add_subdirectory(wasi_crypto)
endif()
