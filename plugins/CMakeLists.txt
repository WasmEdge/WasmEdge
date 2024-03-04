# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

if(WASMEDGE_PLUGIN_WASI_HTTP)
  add_subdirectory(wasi_http)
endif()

if(WASMEDGE_PLUGIN_WASI_NN_BACKEND)
  add_subdirectory(wasi_nn)
endif()

if(WASMEDGE_PLUGIN_WASI_CRYPTO)
  add_subdirectory(wasi_crypto)
endif()

if(WASMEDGE_PLUGIN_IMAGE)
  # Only Linux and MacOS support wasmedge_image now.
  if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_subdirectory(wasmedge_image)
  else()
    message(WARNING "Only Linux and Darwin platforms support WasmEdge_Image plug-in now.")
  endif()
endif()

if(WASMEDGE_PLUGIN_TENSORFLOW)
  # Only Linux and MacOS support wasmedge_tensorflow now.
  if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_subdirectory(wasmedge_tensorflow)
  else()
    message(WARNING "Only Linux and Darwin platforms support WasmEdge_Tensorflow plug-in now.")
  endif()
endif()

if(WASMEDGE_PLUGIN_TENSORFLOWLITE)
  # Only Linux and MacOS support wasmedge_tensorflowlite now.
  if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_subdirectory(wasmedge_tensorflowlite)
  else()
    message(WARNING "Only Linux and Darwin platforms support WasmEdge_TensorflowLite plug-in now.")
  endif()
endif()

if(WASMEDGE_PLUGIN_PROCESS)
  # Only Linux systems support wasmedge_process now.
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_subdirectory(wasmedge_process)
  else()
    message(WARNING "Only Linux platforms support WasmEdge_Process plug-in now.")
  endif()
endif()

if(WASMEDGE_PLUGIN_WASM_BPF)
  # Only Linux systems support wasm_bpf now.
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_subdirectory(wasm_bpf)
  else()
    message(WARNING "Only Linux platforms support wasm_bpf plug-in now.")
  endif()
endif()

if(WASMEDGE_PLUGIN_WASI_OCR)
  add_subdirectory(wasi_ocr)
endif()

if(WASMEDGE_PLUGIN_OPENCVMINI)
  # Only Linux and MacOS support wasmedge_opencvmini now.
  if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_subdirectory(wasmedge_opencvmini)
  else()
    message(WARNING "Only Linux and Darwin platforms support WasmEdge_OpenCVMini plug-in now.")
  endif()
endif()

if(WASMEDGE_PLUGIN_WASI_LOGGING)
  add_subdirectory(wasi_logging)
endif()

if(WASMEDGE_PLUGIN_RUSTLS)
  add_subdirectory(wasmedge_rustls)
endif()

if(WASMEDGE_PLUGIN_ZLIB)
  add_subdirectory(wasmedge_zlib)
endif()

if(WASMEDGE_PLUGIN_FFMPEG)
  add_subdirectory(wasmedge_ffmpeg)
endif()
