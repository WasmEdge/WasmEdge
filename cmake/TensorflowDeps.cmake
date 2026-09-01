# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

include_guard()

include(FetchContent)

# Function for preparing TensorFlow-related variables.
if(NOT WASMEDGE_DEPS_VERSION)
  set(WASMEDGE_DEPS_VERSION "TF-2.21.0-CC")
endif()
function(wasmedge_setup_tf_variables)
  # Set the system name and hash of TF and TFLite releases.
  if(ANDROID)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
      set(WASMEDGE_TENSORFLOW_SYSTEM_NAME "android_aarch64" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH "3937cf406d198f633f0a5af9a9e8f599e9de4c26654cd6ba3a30ec55e910c7c2" PARENT_SCOPE)
    else()
      message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
  elseif(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
      set(WASMEDGE_TENSORFLOW_SYSTEM_NAME "darwin_x86_64" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TF_HASH "e8c71d1d741435ab0c23d8deb5421a9f24c7cd375a5e6855ff7e9a8b12b159a2" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH "b4596fd34d2c26ae26b2216561c2cf506684c9ebc3162a4c9a4ccc974727f5c6" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
      set(WASMEDGE_TENSORFLOW_SYSTEM_NAME "darwin_arm64" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TF_HASH "ab0a8810f1a4795b1a761ddd1e1ef8e29faaf48c0e904fdc1cfde222fd615c2a" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH "162d3be33b28a4df4dd54e2bdc6648176e6594c06d41205ab65232e559c800b5" PARENT_SCOPE)
    else()
      message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
  elseif(UNIX)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
      set(WASMEDGE_TENSORFLOW_SYSTEM_NAME "manylinux_2_28_x86_64" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TF_HASH "a18d5366bd20b1c6f04748a96f016e07aa8d72cb9578c749d47421d28a6842fa" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH "9365151c3b63dcd3587d562406a02fdb55047c4c364b7be362a71e88393cbdc9" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
      set(WASMEDGE_TENSORFLOW_SYSTEM_NAME "manylinux_2_28_aarch64" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TF_HASH "d8f9536ea963e75f7ee7ea1cc5a305578dc8a205af10e7c0c7493449a7c20f2a" PARENT_SCOPE)
      set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH "e4b97a428c32b9f677512d3ae1b96ebe81966d5e229255ebb81d22d88f7ed21c" PARENT_SCOPE)
    else()
      message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
  else()
    message(FATAL_ERROR "Unsupported system: ${CMAKE_SYSTEM_NAME}")
  endif()
endfunction()

# Function for preparing TensorFlow headers.
function(wasmedge_setup_tf_headers)
  # Fetch Tensorflow-deps repository.
  if(NOT wasmedge_tensorflow_header_SOURCE_DIR)
    message(STATUS "Downloading WasmEdge-tensorflow-deps repository")
    FetchContent_Declare(
      wasmedge_tensorflow_header
      GIT_REPOSITORY https://github.com/second-state/WasmEdge-tensorflow-deps.git
      GIT_TAG ${WASMEDGE_DEPS_VERSION}
    )
    FetchContent_MakeAvailable(wasmedge_tensorflow_header)
    message(STATUS "Downloading WasmEdge-tensorflow-deps repository -- done")
  endif()

  # Setup Tensorflow headers source.
  set(WASMEDGE_TENSORFLOW_DEPS_HEADERS ${wasmedge_tensorflow_header_SOURCE_DIR} PARENT_SCOPE)
endfunction()

# Function for preparing the TensorFlow-Lite library.
function(wasmedge_setup_tflite_lib)
  wasmedge_setup_tf_variables()
  # Fetch Tensorflow-lite pre-built library.
  if(NOT wasmedge_tensorflow_lib_tflite_SOURCE_DIR)
    message(STATUS "Downloading libtensorflowlite dependency")
    FetchContent_Declare(
      wasmedge_tensorflow_lib_tflite
      URL "https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/${WASMEDGE_DEPS_VERSION}/WasmEdge-tensorflow-deps-TFLite-${WASMEDGE_DEPS_VERSION}-${WASMEDGE_TENSORFLOW_SYSTEM_NAME}.tar.gz"
      URL_HASH "SHA256=${WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH}"
    )
    FetchContent_MakeAvailable(wasmedge_tensorflow_lib_tflite)
    message(STATUS "Downloading libtensorflowlite dependency -- done")
  endif()

  # Setup Tensorflow-lite pre-built library.
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

# Function for preparing the TensorFlow library.
function(wasmedge_setup_tf_lib)
  wasmedge_setup_tf_variables()
  # Fetch Tensorflow pre-built library.
  if(NOT wasmedge_tensorflow_lib_tf_SOURCE_DIR)
    message(STATUS "Downloading libtensorflow dependency")
    FetchContent_Declare(
      wasmedge_tensorflow_lib_tf
      URL "https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/${WASMEDGE_DEPS_VERSION}/WasmEdge-tensorflow-deps-TF-${WASMEDGE_DEPS_VERSION}-${WASMEDGE_TENSORFLOW_SYSTEM_NAME}.tar.gz"
      URL_HASH "SHA256=${WASMEDGE_TENSORFLOW_DEPS_TF_HASH}"
    )
    FetchContent_MakeAvailable(wasmedge_tensorflow_lib_tf)
    message(STATUS "Downloading libtensorflow dependency -- done")
  endif()

  # Setup Tensorflow pre-built library.
  if(ANDROID)
  elseif(APPLE)
    set(WASMEDGE_TENSORFLOW_DEPS_TF_LIB
      "${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_cc.2.dylib"
      "${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_framework.2.dylib"
      PARENT_SCOPE
    )
  elseif(UNIX)
    set(WASMEDGE_TENSORFLOW_DEPS_TF_LIB
      "${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_cc.so.2"
      "${wasmedge_tensorflow_lib_tf_SOURCE_DIR}/libtensorflow_framework.so.2"
      PARENT_SCOPE
    )
  endif()
endfunction()

# Function for preparing the TensorFlow dependency.
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

# Function for preparing the TensorFlow-Lite dependency.
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
