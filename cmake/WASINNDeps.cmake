# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

include(FetchContent)

if(NOT WASMEDGE_DEPS_VERSION)
  set(WASMEDGE_DEPS_VERSION "TF-2.12.0-CC")
endif()

function(wasmedge_setup_tf_variables)
  # Set the system name and hash of TF and TFLite releases.
  if(ANDROID)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
      set(WASMEDGE_TENSORFLOW_SYSTEM_NAME "android_aarch64" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH "2d7dcd7381479d9ffc0968ea66e24a5207b404c7f2ccbdddec6f2a4d6f9813f2" PARENT_SCOPE)
    elseif()
      message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
  elseif(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
      set(WASMEDGE_TENSORFLOW_SYSTEM_NAME "darwin_x86_64" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TF_HASH "60da72a093cf65d733ca2cb9f331356a1637acfe1645050809bd0cf056b1520f" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH "04b58f4b97220633a8e299a63aba73d9a1f228904081e7d5f18e78d1e38d5f00" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
      set(WASMEDGE_TENSORFLOW_SYSTEM_NAME "darwin_arm64" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TF_HASH "2ede6d96c7563eb826331469d7d0a1f51c9b1ca311f4398d841f679a5b96705a" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH "cb4562a80ac2067bdabe2464b80e129b9d8ddc6d97ad1a2d7215e06a1e1e8cda" PARENT_SCOPE)
    else()
      message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
  elseif(UNIX)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
      set(WASMEDGE_TENSORFLOW_SYSTEM_NAME "manylinux2014_x86_64" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TF_HASH "266465acd642a9d2d80e56c93aa0a255597bfb3034a826bb2225e61f2bebe2e2" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH "110a06bcda1fdc3e744b1728157b66981e235de130f3a34755684e6adcf08341" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
      set(WASMEDGE_TENSORFLOW_SYSTEM_NAME "manylinux2014_aarch64" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TF_HASH "9c15a3aeeda614c9677fe8980d8fa2cd9600072c4701b8a8189225855b9ca1a8" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH "672b81d3f4b5a6c25dc9bbc3b8c6ac1c0357cfab8105b2a85b8bb8c0b59afcb4" PARENT_SCOPE)
    else()
      message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
  else()
    message(FATAL_ERROR "Unsupported system: ${CMAKE_SYSTEM_NAME}")
  endif()
endfunction()

function(wasmedge_setup_tf_headers)
  FetchContent_Declare(
    wasmedge_tensorflow_header
    GIT_REPOSITORY https://github.com/second-state/WasmEdge-tensorflow-deps.git
    GIT_TAG ${WASMEDGE_DEPS_VERSION}
  )
  FetchContent_GetProperties(wasmedge_tensorflow_header)

  if(NOT wasmedge_tensorflow_header_POPULATED)
    message(STATUS "Fetching WasmEdge-tensorflow-deps repository")
    FetchContent_Populate(wasmedge_tensorflow_header)
    message(STATUS "Fetching WasmEdge-tensorflow-deps repository - done")
  endif()
  set(WASMEDGE_TENSORFLOW_DEPS_HEADERS ${wasmedge_tensorflow_header_SOURCE_DIR} PARENT_SCOPE)
endfunction()

function(wasmedge_setup_tflite_lib)
  wasmedge_setup_tf_variables()
  # Fetch Tensorflow-lite library.
  FetchContent_Declare(
    wasmedge_tensorflow_lib_tflite
    URL "https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/${WASMEDGE_DEPS_VERSION}/WasmEdge-tensorflow-deps-TFLite-${WASMEDGE_DEPS_VERSION}-${WASMEDGE_TENSORFLOW_SYSTEM_NAME}.tar.gz"
    URL_HASH "SHA256=${WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH}"
  )
  FetchContent_GetProperties(wasmedge_tensorflow_lib_tflite)

  if(NOT wasmedge_tensorflow_lib_tflite_POPULATED)
    message(STATUS "Downloading dependency: libtensorflowlite")
    FetchContent_Populate(wasmedge_tensorflow_lib_tflite)
    message(STATUS "Downloading dependency: libtensorflowlite - done")
  endif()

  # Setup Tensorflow-lite library.
  if(ANDROID)
    set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_LIB
      "${wasmedge_tensorflow_lib_tflite_SOURCE_DIR}/libtensorflowlite_c.so"
      PARENT_SCOPE
    )
  elseif(APPLE)
    set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_LIB
      "${wasmedge_tensorflow_lib_tflite_SOURCE_DIR}/libtensorflowlite_c.dylib"
      "${wasmedge_tensorflow_lib_tflite_SOURCE_DIR}/libtensorflowlite_flex.dylib"
      PARENT_SCOPE
    )
  elseif(UNIX)
    set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_LIB
      "${wasmedge_tensorflow_lib_tflite_SOURCE_DIR}/libtensorflowlite_c.so"
      "${wasmedge_tensorflow_lib_tflite_SOURCE_DIR}/libtensorflowlite_flex.so"
      PARENT_SCOPE
    )
  endif()
endfunction()

function(wasmedge_setup_tf_lib)
  wasmedge_setup_tf_variables()
  # Fetch Tensorflow-lite library.
  FetchContent_Declare(
    wasmedge_tensorflow_lib_tf
    URL "https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/${WASMEDGE_DEPS_VERSION}/WasmEdge-tensorflow-deps-TF-${WASMEDGE_DEPS_VERSION}-${WASMEDGE_TENSORFLOW_SYSTEM_NAME}.tar.gz"
    URL_HASH "SHA256=${WASMEDGE_TENSORFLOW_DEPS_TF_HASH}"
  )
  FetchContent_GetProperties(wasmedge_tensorflow_lib_tf)

  if(NOT wasmedge_tensorflow_lib_tf_POPULATED)
    message(STATUS "Downloading dependency: libtensorflow")
    FetchContent_Populate(wasmedge_tensorflow_lib_tf)
    if(APPLE)
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink libtensorflow_cc.2.12.0.dylib ${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_cc.2.dylib
        COMMAND ${CMAKE_COMMAND} -E create_symlink libtensorflow_cc.2.dylib ${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_cc.dylib
        COMMAND ${CMAKE_COMMAND} -E create_symlink libtensorflow_framework.2.12.0.dylib ${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_framework.2.dylib
        COMMAND ${CMAKE_COMMAND} -E create_symlink libtensorflow_framework.2.dylib ${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_framework.dylib
      )
    else()
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink libtensorflow_cc.so.2.12.0 ${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_cc.so.2
        COMMAND ${CMAKE_COMMAND} -E create_symlink libtensorflow_cc.so.2 ${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_cc.so
        COMMAND ${CMAKE_COMMAND} -E create_symlink libtensorflow_framework.so.2.12.0 ${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_framework.so.2
        COMMAND ${CMAKE_COMMAND} -E create_symlink libtensorflow_framework.so.2 ${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_framework.so
      )
    endif()
    message(STATUS "Downloading dependency: libtensorflow - done")
  endif()

  if(ANDROID)
  elseif(APPLE)
    set(WASMEDGE_TENSORFLOW_DEPS_TF_LIB
      "${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_cc.2.12.0.dylib"
      "${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_framework.2.12.0.dylib"
      PARENT_SCOPE
    )
  elseif(UNIX)
    set(WASMEDGE_TENSORFLOW_DEPS_TF_LIB
      "${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_cc.so.2.12.0"
      "${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_framework.so.2.12.0"
      PARENT_SCOPE
    )
  endif()
endfunction()

function(wasmedge_setup_wasinn_target target)
  # Add backends building flags.
  foreach(BACKEND ${WASMEDGE_PLUGIN_WASI_NN_BACKEND})
    string(TOLOWER ${BACKEND} BACKEND)
    if(BACKEND STREQUAL "openvino")
      message(STATUS "WASI-NN: Build OpenVINO backend for WASI-NN")
      find_package(OpenVINO REQUIRED)
      add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO)
      list(APPEND WASMEDGE_PLUGIN_WASI_NN_DEPS
        openvino::runtime openvino::runtime::c
      )
    elseif(BACKEND STREQUAL "pytorch")
      message(STATUS "WASI-NN: Build PyTorch backend for WASI-NN")
      find_package(Torch REQUIRED)
      add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH)
      list(APPEND WASMEDGE_PLUGIN_WASI_NN_DEPS
        ${TORCH_LIBRARIES}
      )
    elseif(BACKEND STREQUAL "tensorflowlite")
      message(STATUS "WASI-NN: Build Tensorflow lite backend for WASI-NN")
      add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE)
      wasmedge_setup_tf_headers()
      list(APPEND WASMEDGE_PLUGIN_WASI_NN_INCLUDES
        ${WASMEDGE_TENSORFLOW_DEPS_HEADERS}
      )
      wasmedge_setup_tflite_lib()
      list(APPEND WASMEDGE_PLUGIN_WASI_NN_DEPS
        ${WASMEDGE_TENSORFLOW_DEPS_TFLITE_LIB}
      )
    elseif(BACKEND STREQUAL "ggml")
      message(STATUS "WASI-NN: Build ggml backend for WASI-NN")
      add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML)
    elseif(BACKEND STREQUAL "neuralspeed")
      message(NOTICE "WASI-NN: Neural Speed backend is removed due to the upstream end-of-life.")
    elseif(BACKEND STREQUAL "piper")
      message(STATUS "WASI-NN: Build piper backend for WASI-NN")
      add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER)
      wasmedge_setup_piper()
      list(APPEND WASMEDGE_PLUGIN_WASI_NN_DEPS piper)
    elseif(BACKEND STREQUAL "whisper")
      message(STATUS "WASI-NN: Build whisper.cpp backend for WASI-NN")
      add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER)
    elseif(BACKEND STREQUAL "chattts")
      message(STATUS "WASI-NN: Build chatTTS backend for WASI-NN")
      add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_CHATTTS)
    else()
      # Add the other backends here.
      message(FATAL_ERROR "WASI-NN: backend ${BACKEND} not found or unimplemented.")
    endif()
  endforeach()

  target_include_directories(${target}
    PUBLIC
    ${WASMEDGE_PLUGIN_WASI_NN_INCLUDES}
  )
  target_link_libraries(${target}
    PUBLIC
    ${WASMEDGE_PLUGIN_WASI_NN_DEPS}
  )
endfunction()

function(wasmedge_setup_tf_target target)
  wasmedge_setup_tf_headers()
  wasmedge_setup_tf_lib()
  target_include_directories(${target}
    PUBLIC
    ${WASMEDGE_TENSORFLOW_DEPS_HEADERS}
  )
  target_link_libraries(${target}
    PUBLIC
    ${WASMEDGE_TENSORFLOW_DEPS_TF_LIB}
  )
endfunction()

function(wasmedge_setup_tflite_target target)
  wasmedge_setup_tf_headers()
  wasmedge_setup_tflite_lib()
  target_include_directories(${target}
    PUBLIC
    ${WASMEDGE_TENSORFLOW_DEPS_HEADERS}
  )
  target_link_libraries(${target}
    PUBLIC
    ${WASMEDGE_TENSORFLOW_DEPS_TFLITE_LIB}
  )
endfunction()

function(wasmedge_setup_piper)
  include(Helper)
  wasmedge_setup_spdlog()
  find_package(onnxruntime)
  if(NOT onnxruntime_FOUND)
    find_library(ONNXRUNTIME_LIBRARY onnxruntime)
    if(NOT "${ONNXRUNTIME_LIBRARY}" STREQUAL "ONNXRUNTIME_LIBRARY-NOTFOUND")
      find_path(ONNXRUNTIME_PATH "onnxruntime_cxx_api.h" PATH_SUFFIXES "onnxruntime")
      if(NOT "${ONNXRUNTIME_PATH}" STREQUAL "ONNXRUNTIME_PATH-NOTFOUND")
        set(onnxruntime_FOUND TRUE)
      endif()
    endif()
  endif()
  if(NOT onnxruntime_FOUND)
    message(FATAL_ERROR "Cannot find onnxruntime")
  endif()
  message(STATUS "Downloading piper source")
  include(FetchContent)
  set(BUILD_SHARED_LIBS OFF)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(BUILD_TESTING OFF)
  find_program(GIT_CMD git REQUIRED)
  FetchContent_Declare(
    piper
    GIT_REPOSITORY https://github.com/rhasspy/piper.git
    GIT_TAG 38917ffd8c0e219c6581d73e07b30ef1d572fce1 # 2023.11.14-2
    UPDATE_DISCONNECTED TRUE
    PATCH_COMMAND "${GIT_CMD}" "apply" "${CMAKE_SOURCE_DIR}/plugins/wasi_nn/piper.patch"
  )
  FetchContent_GetProperties(piper)
  if(NOT piper_POPULATED)
    FetchContent_Populate(piper)
    add_subdirectory("${piper_SOURCE_DIR}" "${piper_BINARY_DIR}" EXCLUDE_FROM_ALL)
  endif()
  # suppress src/cpp/piper.cpp:302:29: error: unused parameter ‘config’ [-Werror=unused-parameter]
  target_compile_options(piper PRIVATE -Wno-error=unused-parameter)
endfunction()
