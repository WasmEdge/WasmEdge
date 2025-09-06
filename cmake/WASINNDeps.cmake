# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

include(FetchContent)

# Function of setup WASI-NN target.
function(wasmedge_setup_wasinn_target target)
  cmake_parse_arguments(PARSE_ARGV 1 WASMEDGE_WASINNDEPS_${target} "PLUGINLIB" "" "")
  # `PLUGINLIB` keyword is for the target which is the plugin library.
  # For some dependencies, the fully linking are necessary because the invoking happened in the plugin header.
  # In other cases, the includes are needed only if the target is not the plugin library, such as the tests.

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
      # Add the message of other backends here.
      message(FATAL_ERROR "WASI-NN: backend ${BACKEND} not found or unimplemented.")
    endif()
  endforeach()
endfunction()

# Function of preparing TensorFlow related variables.
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

# Function of preparing TensorFlow headers.
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

# Function of preparing TensorFlow-Lite library.
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

# Function of preparing TensorFlow library.
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
  endif()

  # Setup Tensorflow pre-built library.
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

# Function of preparing TensorFlow dependency.
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

# Function of preparing TensorFlow-Lite dependency.
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

# Function of preparing llama dependency.
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
      GIT_TAG        b6399
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
    # Ignore unused function warnings at common.h in llama.cpp.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(${target}
        PRIVATE
        -Wno-unused-function
      )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
      target_compile_options(${target}
        /wd4305
        /wd4244
      )
    endif()
  endif()
  # Only the plugin library needs to fully linking the dependency.
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

# Function of preparing piper dependency.
function(wasmedge_setup_piper_target target)
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
  if(NOT TARGET piper)
    # setup piper
    message(STATUS "Downloading piper source")
    find_program(GIT_CMD git REQUIRED)
    FetchContent_Declare(
      piper
      GIT_REPOSITORY https://github.com/rhasspy/piper.git
      GIT_TAG 38917ffd8c0e219c6581d73e07b30ef1d572fce1 # 2023.11.14-2
      UPDATE_DISCONNECTED TRUE
      PATCH_COMMAND "${GIT_CMD}" "apply" "${CMAKE_SOURCE_DIR}/plugins/wasi_nn/piper.patch"
    )
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Piper not build shared")
    set(BUILD_TESTING OFF CACHE INTERNAL "Piper not build tests")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "Piper build independent code")
    FetchContent_MakeAvailable(piper)
    message(STATUS "Downloading piper source -- done")
    # suppress src/cpp/piper.cpp:302:29: error: unused parameter ‘config’ [-Werror=unused-parameter]
    target_compile_options(piper PRIVATE -Wno-error=unused-parameter)
  endif()
  wasmedge_setup_simdjson()
  target_link_libraries(${target}
    PRIVATE
    piper
    simdjson::simdjson
  )
endfunction()

# Function of preparing whisper dependency.
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
  # Only the plugin library needs to fully linking the dependency.
  if(WASMEDGE_WASINNDEPS_${target}_PLUGINLIB)
    wasmedge_setup_simdjson()
    target_link_libraries(${target}
      PRIVATE
      whisper
      simdjson::simdjson
    )
  endif()
endfunction()

# Function of preparing MLX library.
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
        GIT_TAG        v0.24.1
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
      GIT_TAG 5de6f65
      GIT_SHALLOW FALSE
    )
    FetchContent_MakeAvailable(tokenizers)
    message(STATUS "Downloading tokenizers source -- done")
    set_property(TARGET tokenizer_cpp_objs PROPERTY POSITION_INDEPENDENT_CODE ON)
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
  # Only the plugin library needs to fully linking the dependency.
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
      GIT_SHALLOW    TRUE
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
