# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

if(WASMEDGE_PLUGIN_FFMPEG)
  add_subdirectory(wasmedge_ffmpeg)
endif()

if(WASMEDGE_PLUGIN_PROCESS)
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_subdirectory(wasmedge_process)
  endif()
endif()

if(WASMEDGE_PLUGIN_ZLIB)
  add_subdirectory(wasmedge_zlib)
endif()

if(WASMEDGE_PLUGIN_WASI_NN_BACKEND)
  add_subdirectory(wasi_nn)
endif()

if(WASMEDGE_PLUGIN_WASI_CRYPTO)
  add_subdirectory(wasi_crypto)
endif()

if(WASMEDGE_PLUGIN_TENSORFLOW)
  if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_subdirectory(wasmedge_tensorflow)
  endif()
endif()

if(WASMEDGE_PLUGIN_TENSORFLOWLITE)
  if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_subdirectory(wasmedge_tensorflowlite)
  endif()
endif()

if(WASMEDGE_PLUGIN_IMAGE)
  if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_subdirectory(wasmedge_image)
  endif()
endif()

if(WASMEDGE_PLUGIN_OPENCVMINI)
  if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_subdirectory(wasmedge_opencvmini)
  endif()
endif()

if(WASMEDGE_PLUGIN_WASM_BPF)
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_subdirectory(wasm_bpf)
  endif()
endif()

if(WASMEDGE_PLUGIN_WASI_LOGGING)
  add_subdirectory(wasi_logging)
endif()

if(WASMEDGE_PLUGIN_RUSTLS)
  add_subdirectory(wasmedge_rustls)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "Darwin")
  add_subdirectory(unittest)
endif()
