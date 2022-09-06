# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

<<<<<<< HEAD
# Only Linux systems support wasmedge_process now.
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  add_subdirectory(wasmedge_process)
endif()

if (WASMEDGE_PLUGIN_WASI_NN_BACKEND)
=======
add_subdirectory(wasmedge_process)
add_subdirectory(wasmedge_thread)
if(WASMEDGE_WASINN_BACKEND)
>>>>>>> [Plugin] split wasmedge thread into a plugin
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
