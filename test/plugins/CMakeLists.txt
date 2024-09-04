# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

# WASI plug-in: WASI-Crypto proposal.
if(WASMEDGE_PLUGIN_WASI_CRYPTO)
  add_subdirectory(wasi_crypto)
endif()

# WASI plug-in: WASI-Logging proposal.
add_subdirectory(wasi_logging)

# WASI plug-in: WASI-NN proposal with backends.
if(WASMEDGE_PLUGIN_WASI_NN_BACKEND)
  add_subdirectory(wasi_nn)
endif()

# WasmEdge plug-in: wasm-bpf.
if(WASMEDGE_PLUGIN_WASM_BPF)
  # Only Linux systems support wasm_bpf now.
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_subdirectory(wasm_bpf)
  else()
    message(WARNING "Only Linux platforms support wasm_bpf plug-in now.")
  endif()
endif()

# WasmEdge plug-in: ffmpeg.
if(WASMEDGE_PLUGIN_FFMPEG)
  add_subdirectory(wasmedge_ffmpeg)
endif()

# WasmEdge plug-in: Image.
if(WASMEDGE_PLUGIN_IMAGE)
  # Only Linux and MacOS support wasmedge_image now.
  if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_subdirectory(wasmedge_image)
  else()
    message(WARNING "Only Linux and Darwin platforms support WasmEdge_Image plug-in now.")
  endif()
endif()

# WasmEdge plug-in: LLMC.
if(WASMEDGE_PLUGIN_LLMC)
  add_subdirectory(wasmedge_llmc)
endif()

# WasmEdge plug-in: OpenCV-mini.
if(WASMEDGE_PLUGIN_OPENCVMINI)
  # Only Linux and MacOS support wasmedge_opencvmini now.
  if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_subdirectory(wasmedge_opencvmini)
  else()
    message(WARNING "Only Linux and Darwin platforms support WasmEdge_OpenCVMini plug-in now.")
  endif()
endif()

# WasmEdge plug-in: Process.
if(WASMEDGE_PLUGIN_PROCESS)
  # Only Linux systems support wasmedge_process now.
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_subdirectory(wasmedge_process)
  else()
    message(WARNING "Only Linux platforms support WasmEdge_Process plug-in now.")
  endif()
endif()

# WasmEdge plug-in: Stable-diffusion.
if(WASMEDGE_PLUGIN_STABLEDIFFUSION)
  # Only Linux and MacOS support wasmedge_stablediffusion now.
  if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_subdirectory(wasmedge_stablediffusion)
  else()
    message(WARNING "Only Linux and Darwin platforms support WasmEdge_StableDiffusion plug-in now.")
  endif()
endif()

# WasmEdge plug-in: TensorFlow.
if(WASMEDGE_PLUGIN_TENSORFLOW)
  # Only Linux and MacOS support wasmedge_tensorflow now.
  if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_subdirectory(wasmedge_tensorflow)
  else()
    message(WARNING "Only Linux and Darwin platforms support WasmEdge_Tensorflow plug-in now.")
  endif()
endif()

# WasmEdge plug-in: TensorFlow-Lite.
if(WASMEDGE_PLUGIN_TENSORFLOWLITE)
  # Only Linux and MacOS support wasmedge_tensorflowlite now.
  if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    add_subdirectory(wasmedge_tensorflowlite)
  else()
    message(WARNING "Only Linux and Darwin platforms support WasmEdge_TensorflowLite plug-in now.")
  endif()
endif()

# WasmEdge plug-in: zlib.
if(WASMEDGE_PLUGIN_ZLIB)
  add_subdirectory(wasmedge_zlib)
endif()

# Plug-in unit tests.
add_subdirectory(unittest)
