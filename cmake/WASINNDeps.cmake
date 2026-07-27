# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

include(FetchContent)

# Function for setting up a WASI-NN target.
function(wasmedge_setup_wasinn_target target)
  cmake_parse_arguments(PARSE_ARGV 1 WASMEDGE_WASINNDEPS_${target} "PLUGINLIB" "" "")
  # The `PLUGINLIB` keyword marks the target that is the plugin library.
  # Some dependencies must be fully linked because invocation happens in the plugin header.
  # In other cases, includes are needed only if the target is not the plugin library, such as the tests.

  foreach(BACKEND ${WASMEDGE_PLUGIN_WASI_NN_BACKEND})
    string(TOLOWER ${BACKEND} BACKEND)
    if(BACKEND STREQUAL "openvino")
      if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
        message(STATUS "WASI-NN: Build OpenVINO backend for WASI-NN")
      endif()
      target_compile_definitions(${target} PUBLIC WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO)
      find_package(OpenVINO REQUIRED)
      target_link_libraries(${target}
        PRIVATE
        openvino::runtime
        openvino::runtime::c
      )
    elseif(BACKEND STREQUAL "openvinogenai")
      if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
        message(STATUS "WASI-NN: Build OpenVINO GenAI backend for WASI-NN")
      endif()
      target_compile_definitions(${target} PUBLIC WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINOGENAI)
      find_package(OpenVINO REQUIRED)
      find_package(OpenVINOGenAI REQUIRED)
      target_link_libraries(${target}
        PRIVATE
        openvino::runtime
        openvino::runtime::c
        openvino::genai
      )
    elseif(BACKEND STREQUAL "pytorch")
      if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
        message(STATUS "WASI-NN: Build PyTorch backend for WASI-NN")
      endif()
      target_compile_definitions(${target} PUBLIC WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH)
      find_package(Torch REQUIRED)
      target_link_libraries(${target}
        PRIVATE
        ${TORCH_LIBRARIES}
      )
      target_compile_options(${target}
        PRIVATE
        -Wno-error=unused-parameter
      )
    elseif(BACKEND STREQUAL "tensorflowlite")
      if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
        message(STATUS "WASI-NN: Build Tensorflow-lite backend for WASI-NN")
      endif()
      target_compile_definitions(${target} PUBLIC WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE)
      wasmedge_setup_tflite_target(${target})
    elseif(BACKEND STREQUAL "ggml")
      if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
        message(STATUS "WASI-NN: Build ggml backend for WASI-NN")
      endif()
      target_compile_definitions(${target} PUBLIC WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML)
      wasmedge_setup_llama_target(${target})
    elseif(BACKEND STREQUAL "neuralspeed")
      message(FATAL_ERROR "WASI-NN: Neural Speed backend is removed due to the upstream end-of-life. Reference: https://github.com/intel/neural-speed")
    elseif(BACKEND STREQUAL "piper")
      if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
        message(STATUS "WASI-NN: Build piper backend for WASI-NN")
      endif()
      target_compile_definitions(${target} PUBLIC WASMEDGE_PLUGIN_WASI_NN_BACKEND_PIPER)
      wasmedge_setup_piper_target(${target})
    elseif(BACKEND STREQUAL "whisper")
      if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
        message(STATUS "WASI-NN: Build whisper.cpp backend for WASI-NN")
      endif()
      target_compile_definitions(${target} PUBLIC WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER)
      wasmedge_setup_whisper_target(${target})
    elseif(BACKEND STREQUAL "chattts")
      if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
        message(STATUS "WASI-NN: Build chatTTS backend for WASI-NN")
      endif()
      target_compile_definitions(${target} PUBLIC WASMEDGE_PLUGIN_WASI_NN_BACKEND_CHATTTS)
      wasmedge_setup_simdjson()
      find_package(Python3 COMPONENTS Interpreter Development)
      if(NOT Python3_FOUND)
        message(FATAL_ERROR "Can not find python3.")
      endif()
      target_compile_definitions(${target}
        PRIVATE
        PYTHON_LIB_PATH="${Python3_LIBRARIES}"
      )
      target_include_directories(${target}
        PRIVATE
        ${Python3_INCLUDE_DIRS}
      )
      target_link_directories(${target}
        PRIVATE
        ${Python3_RUNTIME_LIBRARY_DIRS}
      )
      target_link_libraries(${target}
        PRIVATE
        ${Python3_LIBRARIES}
        simdjson::simdjson
      )
    elseif(BACKEND STREQUAL "mlx")
      wasmedge_setup_simdjson()
      if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
        message(STATUS "WASI-NN: Build MLX backend for WASI-NN")
      endif()
      target_compile_definitions(${target} PUBLIC WASMEDGE_PLUGIN_WASI_NN_BACKEND_MLX)
      wasmedge_setup_mlx_target(${target})
    elseif(BACKEND STREQUAL "bitnet")
      if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
        message(STATUS "WASI-NN: Build bitnet.cpp backend for WASI-NN")
      endif()
      target_compile_definitions(${target} PUBLIC WASMEDGE_PLUGIN_WASI_NN_BACKEND_BITNET)
      wasmedge_setup_bitnet_target(${target})
    else()
      # Add messages for other backends here.
      message(FATAL_ERROR "WASI-NN: backend ${BACKEND} not found or unimplemented.")
    endif()
  endforeach()
endfunction()

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

# Function for preparing the llama dependency.
function(wasmedge_setup_llama_target target)
  if(NOT TARGET llama)
    # llama.cpp options
    # Disable warnings and debug messages
    set(LLAMA_ALL_WARNINGS OFF)
    # Disable curl dependency
    set(LLAMA_CURL OFF)
    set(LLAMA_METAL_NDEBUG ON)
    set(LLAMA_BUILD_COMMON ON)
    set(LLAMA_BUILD_TOOLS ON)
    set(GGML_ACCELERATE OFF)
    set(GGML_AMX OFF)
    set(GGML_OPENMP OFF)
    set(BUILD_SHARED_LIBS OFF)

    if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_NATIVE)
      message(STATUS "WASI-NN GGML LLAMA backend: Enable GGML_NATIVE(AVX/AVX2/FMA/F16C)")
      set(GGML_NATIVE ON)
    else()
      message(STATUS "WASI-NN GGML LLAMA backend: Disable GGML_NATIVE(AVX/AVX2/FMA/F16C)")
      set(GGML_NATIVE OFF)
      set(GGML_AVX OFF)
      set(GGML_AVX2 OFF)
      set(GGML_FMA OFF)
      set(GGML_F16C OFF)
    endif()

    if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_CUBLAS)
      message(STATUS "WASI-NN GGML LLAMA backend: Enable GGML_CUDA")
      set(GGML_CUDA ON)
      # We need to set GGML_USE_CUDA for clip from llava.
      add_compile_definitions(GGML_USE_CUDA)
    else()
      message(STATUS "WASI-NN GGML LLAMA backend: Disable GGML_CUDA")
      set(GGML_CUDA OFF)
    endif()

    if (WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_BLAS)
      message(STATUS "WASI-NN GGML LLAMA backend: Enable GGML_BLAS")
      set(GGML_BLAS ON)
      set(GGML_BLAS_VENDOR "OpenBLAS")
    else()
      message(STATUS "WASI-NN GGML LLAMA backend: Disable GGML_BLAS")
      set(GGML_BLAS OFF)
    endif()

    if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" AND WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_METAL)
      message(STATUS "WASI-NN GGML LLAMA backend: Enable GGML_METAL")
      set(GGML_METAL ON)
      set(GGML_METAL_EMBED_LIBRARY ON)
    else()
      message(STATUS "WASI-NN GGML LLAMA backend: Disable GGML_METAL")
      set(GGML_METAL OFF)
    endif()

    # setup llama.cpp
    message(STATUS "Downloading llama.cpp source")
    include(FetchContent)
    FetchContent_Declare(
      llama
      GIT_REPOSITORY https://github.com/ggml-org/llama.cpp.git
      GIT_TAG        a29e4c0b7b23e020107058480dabbe03b7cba6e1  # b8757
      GIT_SHALLOW    FALSE
    )
    FetchContent_MakeAvailable(llama)
    message(STATUS "Downloading llama.cpp source -- done")
    set_property(TARGET common PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET ggml PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET ggml-base PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET ggml-cpu PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET llama PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET mtmd PROPERTY POSITION_INDEPENDENT_CODE ON)
    if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_CUBLAS)
      set_property(TARGET ggml-cuda PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()

    # Reach llama.cpp's headers through -isystem / -external:I. They are pulled
    # into this plugin's own translation units, so without this they compile
    # under WASMEDGE_CFLAGS and trip -Werror on clang-cl, where -Wall means
    # -Weverything.
    foreach(LLAMA_TARGET IN ITEMS
        common ggml ggml-base ggml-cpu ggml-cuda llama mtmd)
      wasmedge_mark_system_includes(${LLAMA_TARGET})
    endforeach()
  endif()
  # Ignore unused function warnings in common.h in llama.cpp.
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${target}
      PRIVATE
      -Wno-error=unused-function
    )
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # The GGML sources index vectors with int, which clang-cl reports under
    # -Weverything. Keep the diagnostics, but do not fail the build on them.
    target_compile_options(${target}
      PRIVATE
      -Wno-error=unused-function
      -Wno-error=implicit-float-conversion
      -Wno-error=documentation
      -Wno-error=unused-template
      -Wno-error=sign-conversion
      -Wno-error=extra-semi-stmt
    )
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${target}
      PRIVATE
      /wd4305
      /wd4244
    )
  endif()
  # Only the plugin library needs to fully link the dependency.
  if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
    wasmedge_setup_simdjson()
    target_link_libraries(${target}
      PRIVATE
      common
      simdjson::simdjson
      mtmd
    )
  endif()
endfunction()

# Function for preparing the piper dependency.
function(wasmedge_setup_piper_target target)
  if(NOT TARGET piper)
    # setup piper
    if(DEFINED PIPER_ROOT)
      message(STATUS "Build: Using pre-built Piper from ${PIPER_ROOT}")
      add_library(piper STATIC IMPORTED)
      set(_OLD_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
      find_library(
        PIPER_LIB_PATH
        NAMES piper libpiper
        PATHS "${PIPER_ROOT}"
        NO_DEFAULT_PATH REQUIRED)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ${_OLD_SUFFIXES})
      set_target_properties(
        piper PROPERTIES IMPORTED_LOCATION "${PIPER_LIB_PATH}"
                         INTERFACE_INCLUDE_DIRECTORIES "${PIPER_ROOT}/include")
    else()
      message(STATUS "Downloading piper source")
      find_program(GIT_CMD git REQUIRED)
      FetchContent_Declare(
        piper
        GIT_REPOSITORY https://github.com/OHF-Voice/piper1-gpl.git
        GIT_TAG 32b95f8c1f0dc0ce27a6acd1143de331f61af777
        UPDATE_DISCONNECTED TRUE
        SOURCE_SUBDIR libpiper
        PATCH_COMMAND "${GIT_CMD}" "apply" "${CMAKE_SOURCE_DIR}/utils/wasi-nn/libpiper.patch"
      )
      set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Piper not build shared")
      set(BUILD_TESTING OFF CACHE INTERNAL "Piper not build tests")
      set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "Piper build independent code")
      FetchContent_MakeAvailable(piper)
      message(STATUS "Downloading piper source -- done")
      # suppress src/cpp/piper.cpp:302:29: error: unused parameter ‘config’ [-Werror=unused-parameter]
      target_compile_options(piper PRIVATE -Wno-error=unused-parameter)
    endif()
  endif()
  wasmedge_setup_simdjson()
  target_link_libraries(${target}
    PRIVATE
    piper
    simdjson::simdjson
  )
endfunction()

# Function for preparing the whisper dependency.
function(wasmedge_setup_whisper_target target)
  if(NOT TARGET whisper)
    # setup whisper
    FetchContent_Declare(
      whisper
      GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
      GIT_TAG        69339af2d104802f3f201fd419163defba52890e
      GIT_SHALLOW    FALSE
    )
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Whisper not build shared")
    set(GGML_OPENMP OFF)
    set(GGML_ACCELERATE OFF)
    set(GGML_BLAS OFF)
    if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" AND WASMEDGE_PLUGIN_WASI_NN_WHISPER_METAL)
      message(STATUS "WASI-NN Whisper backend: Enable GGML_METAL")
      set(GGML_METAL ON)
      set(GGML_METAL_EMBED_LIBRARY ON)
    else()
      message(STATUS "WASI-NN Whisper backend: Disable GGML_METAL")
      set(GGML_METAL OFF)
    endif()
    if(WASMEDGE_PLUGIN_WASI_NN_WHISPER_CUDA)
      message(STATUS "WASI-NN Whisper backend: Enable GGML_CUDA")
      set(GGML_CUDA ON)
    else()
      message(STATUS "WASI-NN Whisper backend: Disable GGML_CUDA")
      set(GGML_CUDA OFF)
    endif()
    FetchContent_MakeAvailable(whisper)
    set_property(TARGET whisper PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET ggml PROPERTY POSITION_INDEPENDENT_CODE ON)
    target_include_directories(whisper
      PUBLIC
      ${whisper_SOURCE_DIR}
    )
  endif()
  # Only the plugin library needs to fully link the dependency.
  if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
    wasmedge_setup_simdjson()
    target_link_libraries(${target}
      PRIVATE
      whisper
      simdjson::simdjson
    )
  endif()
endfunction()

# Function for preparing the MLX library.
function(wasmedge_setup_mlx_target target)
  find_package(MLX CONFIG)
  if(MLX_FOUND)
    message(STATUS "Found MLX: ${MLX_INCLUDE_DIRS}")
  else()
    find_library(ACCELERATE_LIBRARY Accelerate)
    find_library(METAL_LIB Metal)
    find_library(FOUNDATION_LIB Foundation)
    find_library(QUARTZ_LIB QuartzCore)
    if(NOT TARGET mlx)
      # setup MLX
      message(STATUS "Downloading MLX source")
      FetchContent_Declare(
        mlx
        GIT_REPOSITORY https://github.com/ml-explore/mlx.git
        GIT_TAG        aba899cef8c9188f92eb12b173fbe5a7ef62d4aa  # v0.24.1
        GIT_SHALLOW    FALSE
      )
      set(MLX_BUILD_GGUF OFF)
      set(MLX_BUILD_TESTS OFF)
      set(MLX_BUILD_EXAMPLES OFF)
      set(BUILD_SHARED_LIBS OFF)
      FetchContent_MakeAvailable(mlx)
      message(STATUS "Downloading MLX source -- done")
      set_property(TARGET mlx PROPERTY POSITION_INDEPENDENT_CODE ON)
      set_target_properties(mlx PROPERTIES
        INTERFACE_LINK_LIBRARIES "$<BUILD_INTERFACE:fmt::fmt-header-only>"
      )
      target_link_libraries(mlx
        PUBLIC
        ${ACCELERATE_LIBRARY}
        ${METAL_LIB}
        ${FOUNDATION_LIB}
        ${QUARTZ_LIB}
      )
      target_compile_options(mlx
        PUBLIC
        -Wno-unused-parameter
        -Wno-deprecated-copy
        -Wno-format
      )
    endif()
  endif()

  if(NOT TARGET tokenizers_cpp)
    # setup tokenizers
    message(STATUS "Downloading tokenizers source")
    FetchContent_Declare(
      tokenizers
      GIT_REPOSITORY https://github.com/mlc-ai/tokenizers-cpp.git
      GIT_TAG 55d53aa
      GIT_SHALLOW FALSE
    )
    FetchContent_MakeAvailable(tokenizers)
    message(STATUS "Downloading tokenizers source -- done")
    set_property(TARGET tokenizers_cpp PROPERTY POSITION_INDEPENDENT_CODE ON)
  endif()

  if(NOT TARGET gguflib)
    # setup gguflib
    message(STATUS "Downloading gguflib source")
    FetchContent_Declare(
      gguflib
      GIT_REPOSITORY https://github.com/antirez/gguf-tools/
      GIT_TAG af7d88d808a7608a33723fba067036202910acb3
      GIT_SHALLOW FALSE
    )
    FetchContent_MakeAvailable(gguflib)
    message(STATUS "Downloading gguflib source -- done")
    add_library(gguflib
      STATIC
      ${gguflib_SOURCE_DIR}/fp16.c
      ${gguflib_SOURCE_DIR}/gguflib.c
    )
    set_target_properties(gguflib PROPERTIES LINKER_LANGUAGE CXX)
  endif()

  find_package(ZLIB REQUIRED)

  find_program(FFMPEG_EXECUTABLE ffmpeg)

  # Only the plugin library needs to fully link the dependency.
  if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
    wasmedge_setup_simdjson()
    target_include_directories(${target}
      PRIVATE
      ${tokenizers_SOURCE_DIR}/include
    )
    target_include_directories(${target}
      SYSTEM PRIVATE
      ${CMAKE_SOURCE_DIR}/plugins/wasi_nn/MLX
      ${MLX_INCLUDE_DIRS}
      $<BUILD_INTERFACE:${gguflib_SOURCE_DIR}>
    )
    target_link_libraries(${target}
      PRIVATE
      tokenizers_cpp
      ${MLX_LIBRARIES}
      gguflib
      mlx
      simdjson::simdjson
      ZLIB::ZLIB
    )
  endif()
endfunction()

function(wasmedge_setup_bitnet_target target)
  if(NOT TARGET llama)

    set(BUILD_SHARED_LIBS OFF)
    set(LLAMA_BUILD_TESTS OFF)
    set(LLAMA_BUILD_EXAMPLES OFF)
    set(LLAMA_BUILD_SERVER OFF)
    set(LLAMA_ALL_WARNINGS OFF)
    set(LLAMA_CURL OFF)
    set(GGML_ACCELERATE OFF)
    set(GGML_BLAS OFF)
    set(GGML_OPENMP OFF)

    if(WASMEDGE_PLUGIN_WASI_NN_BITNET_ARM_TL1)
      message(STATUS "WASI-NN BitNet backend: Enable ARM TL1")
      set(BITNET_ARM_TL1 ON)
    endif()
    if(WASMEDGE_PLUGIN_WASI_NN_BITNET_X86_TL2)
      message(STATUS "WASI-NN BitNet backend: Enable x86 TL2")
      set(BITNET_X86_TL2 ON)
    endif()
    if(WASMEDGE_PLUGIN_WASI_NN_BITNET_ARM_TL1 AND WASMEDGE_PLUGIN_WASI_NN_BITNET_X86_TL2)
      message(FATAL_ERROR "WASI-NN BitNet backend: Both ARM_TL1 or X86_TL2 optimizations cannot be enabled simultaneously")
    endif()

    set(BITNET_PATCH_ARGS "")
    if(WASMEDGE_PLUGIN_WASI_NN_BITNET_ARM_TL1)
        message(STATUS "WASI-NN BitNet Backend: Preparing patch for ARM TL1 build.")
        set(BITNET_PATCH_ARGS
            PATCH_COMMAND ${GIT_CMD} apply ${CMAKE_SOURCE_DIR}/plugins/wasi_nn/bitnet.patch
        )
    endif()

    message(STATUS "Fetching BitNet.cpp source...")
    FetchContent_Declare(
      bitnet
      GIT_REPOSITORY https://github.com/microsoft/BitNet.git
      GIT_TAG        404980e
      GIT_SHALLOW    FALSE
      ${BITNET_PATCH_ARGS}
    )

    # download/checkout and makes the source available but does NOT configure the project.
    FetchContent_Populate(bitnet)
    message(STATUS "Fetching BitNet.cpp source -- done")

    if(WASMEDGE_PLUGIN_WASI_NN_BITNET_ARM_TL1 OR WASMEDGE_PLUGIN_WASI_NN_BITNET_X86_TL2)
        message(STATUS "Pre-generating BitNet kernel headers...")
        find_package(Python3 COMPONENTS Interpreter REQUIRED)

        if(WASMEDGE_PLUGIN_WASI_NN_BITNET_ARM_TL1)
            set(CODEGEN_SCRIPT ${bitnet_SOURCE_DIR}/utils/codegen_tl1.py)
            set(CODEGEN_ARGS --model bitnet_b1_58-3B --BM 160,320,320 --BK 64,128,64 --bm 32,64,32)
        else() # Must be X86_TL2
            set(CODEGEN_SCRIPT ${bitnet_SOURCE_DIR}/utils/codegen_tl2.py)
            set(CODEGEN_ARGS --model bitnet_b1_58-3B --BM 160,320,320 --BK 96,96,96 --bm 32,32,32)
        endif()

        execute_process(
            COMMAND ${Python3_EXECUTABLE} ${CODEGEN_SCRIPT} ${CODEGEN_ARGS}
            WORKING_DIRECTORY ${bitnet_SOURCE_DIR}
            RESULT_VARIABLE GEN_RESULT
        )

        if(NOT GEN_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to pre-generate BitNet kernels.")
        else()
            message(STATUS "Pre-generating BitNet kernel headers -- done")
        endif()
    endif()

    add_subdirectory(${bitnet_SOURCE_DIR} ${bitnet_BINARY_DIR})

    set_property(TARGET llama PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET common PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET ggml PROPERTY POSITION_INDEPENDENT_CODE ON)
    if(TARGET ggml-base)
      set_property(TARGET ggml-base PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
    if(TARGET ggml-cpu)
      set_property(TARGET ggml-cpu PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
    if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_CUBLAS AND TARGET ggml-cuda)
      set_property(TARGET ggml-cuda PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
  endif()

  if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
    wasmedge_setup_simdjson()
    target_link_libraries(${target}
      PRIVATE
      llama
      common
      simdjson::simdjson
    )

  endif()
endfunction()
