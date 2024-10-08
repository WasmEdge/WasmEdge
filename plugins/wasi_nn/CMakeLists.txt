# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgePluginWasiNN
  SHARED
  wasinnenv.cpp
  wasinnfunc.cpp
  wasinnmodule.cpp
  wasinn_openvino.cpp
  wasinn_onnx.cpp
  wasinn_tf.cpp
  wasinn_torch.cpp
  wasinn_tfl.cpp
  wasinn_ggml.cpp
  wasinn_neuralspeed.cpp
  wasinn_piper.cpp
  wasinn_whisper.cpp
  wasinn_chattts.cpp
  wasinn_mlx.cpp
)

include(WASINNDeps)
wasmedge_setup_wasinn_target(wasmedgePluginWasiNN)

foreach(BACKEND ${WASMEDGE_PLUGIN_WASI_NN_BACKEND})
  string(TOLOWER ${BACKEND} BACKEND)
  if(BACKEND STREQUAL "openvino")
    message(STATUS "WASI-NN: Build OpenVINO backend for WASI-NN")
    find_package(OpenVINO REQUIRED)
    target_link_libraries(wasmedgePluginWasiNN
      PUBLIC
      openvino::runtime openvino::runtime::c
    )
  elseif(BACKEND STREQUAL "pytorch")
    message(STATUS "WASI-NN: Build PyTorch backend for WASI-NN")
    find_package(Torch REQUIRED)
    target_link_libraries(wasmedgePluginWasiNN
      PUBLIC
      ${TORCH_LIBRARIES}
    )
  elseif(BACKEND STREQUAL "tensorflowlite")
    message(STATUS "WASI-NN: Build Tensorflow lite backend for WASI-NN")
    wasmedge_setup_tf_headers()
    target_include_directories(wasmedgePluginWasiNN
      PUBLIC
      ${WASMEDGE_TENSORFLOW_DEPS_HEADERS}
    )
    wasmedge_setup_tflite_lib()
    target_link_libraries(wasmedgePluginWasiNN
      PUBLIC
      ${WASMEDGE_TENSORFLOW_DEPS_TFLITE_LIB}
    )
  elseif(BACKEND STREQUAL "ggml")
    message(STATUS "WASI-NN: Build ggml backend for WASI-NN")
    wasmedge_setup_simdjson()
    # llama.cpp options
    # Disable warnings and debug messages
    set(LLAMA_ALL_WARNINGS OFF)
    set(LLAMA_METAL_NDEBUG ON)
    set(GGML_ACCELERATE OFF)
    set(GGML_BLAS OFF)
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
      GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
      GIT_TAG        b3651
      GIT_SHALLOW    FALSE
    )
    FetchContent_MakeAvailable(llama)
    message(STATUS "Downloading llama.cpp source -- done")
    set_property(TARGET ggml PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET common PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET llama PROPERTY POSITION_INDEPENDENT_CODE ON)

    # Setup llava from llama.cpp
    wasmedge_add_library(llava OBJECT
      ${llama_SOURCE_DIR}/examples/llava/clip.cpp
      ${llama_SOURCE_DIR}/examples/llava/llava.cpp
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
      target_compile_options(llava
        PRIVATE
        $<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>
        $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/utf-8>
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4067> # unexpected tokens following preprocessor directive - expected a newline
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4101> # 'identifier' : unreferenced local variable
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4189> # 'identifier' : local variable is initialized but not referenced
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4244> # 'argument' : conversion from 'type1' to 'type2', possible loss of data
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4267> # 'var' : conversion from 'size_t' to 'type', possible loss of data
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4297> # 'function' : function assumed not to throw an exception but does
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4456> # declaration of 'identifier' hides previous local declaration
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4505> # 'function' : unreferenced local function has been removed
      )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
      target_compile_options(llava
        PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-exceptions>
        -Wno-cast-align
        -Wno-cast-qual
        -Wno-float-conversion
        -Wno-implicit-fallthrough
        -Wno-unused-macros
        -Wno-unused-function
        -Wno-unused-variable
      )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(llava
        PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-exceptions>
        -Wno-cast-align
        -Wno-cast-qual
        -Wno-disabled-macro-expansion
        -Wno-float-conversion
        -Wno-implicit-fallthrough
        -Wno-implicit-float-conversion
        -Wno-unused-macros
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-sign-conversion
        -Wno-shorten-64-to-32
        -Wno-implicit-int-conversion
        -Wno-old-style-cast
        -Wno-extra-semi-stmt
        -Wno-format-nonliteral
        -Wno-documentation
        -Wno-unused-template
      )
    endif()
    target_link_libraries(llava PRIVATE ggml llama)
    target_include_directories(llava PUBLIC
      ${llama_SOURCE_DIR}
      ${llama_SOURCE_DIR}/common
      ${llama_SOURCE_DIR}/examples/llava
    )
    target_link_libraries(wasmedgePluginWasiNN
      PRIVATE
      common
      simdjson::simdjson
      llava
    )
  elseif(BACKEND STREQUAL "neuralspeed")
    message(FATAL_ERROR "WASI-NN: Neural Speed backend is removed due to the upstream end-of-life. Reference: https://github.com/intel/neural-speed")
  elseif(BACKEND STREQUAL "chattts")
    message(STATUS "WASI-NN: Build chatTTS backend for WASI-NN")
    wasmedge_setup_simdjson()
    find_package(Python3 COMPONENTS Interpreter Development)
    if(NOT Python3_FOUND)
      message(FATAL_ERROR "Can not find python3.")
    endif()
    target_compile_definitions(wasmedgePluginWasiNN
      PRIVATE
      PYTHON_LIB_PATH="${Python3_LIBRARIES}"
    )
    target_include_directories(wasmedgePluginWasiNN
      PUBLIC
      ${Python3_INCLUDE_DIRS}
    )
    target_link_directories(wasmedgePluginWasiNN
      PRIVATE
      ${Python3_RUNTIME_LIBRARY_DIRS}
    )
    target_link_libraries(wasmedgePluginWasiNN
      PRIVATE
      ${Python3_LIBRARIES}
      simdjson::simdjson
    )
  elseif(BACKEND STREQUAL "piper")
    message(STATUS "WASI-NN: Build piper backend for WASI-NN")
    wasmedge_setup_simdjson()
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
    target_link_libraries(wasmedgePluginWasiNN
      PRIVATE
      piper
      simdjson::simdjson
    )
  elseif(BACKEND STREQUAL "whisper")
    message(STATUS "WASI-NN: Build whisper.cpp backend for WASI-NN")
    wasmedge_setup_simdjson()
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
    FetchContent_Declare(
      whisper
      GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
      GIT_TAG        69339af2d104802f3f201fd419163defba52890e
      GIT_SHALLOW    FALSE
    )
    FetchContent_MakeAvailable(whisper)
    set_property(TARGET whisper PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET ggml PROPERTY POSITION_INDEPENDENT_CODE ON)
    target_include_directories(wasmedgePluginWasiNN
      PUBLIC
      ${whisper_SOURCE_DIR}
    )
    target_link_libraries(wasmedgePluginWasiNN
      PRIVATE 
      whisper
      simdjson::simdjson
    )
  elseif(BACKEND STREQUAL "mlx")
    message(STATUS "WASI-NN: Build MLX backend for WASI-NN")
    wasmedge_setup_simdjson()
    target_sources(wasmedgePluginWasiNN 
      PRIVATE 
      MLX/prompt/prompt.cpp
      MLX/model/transformer.cpp
      MLX/model/converter.cpp
      MLX/model/utils.cpp
      MLX/model/registry.cpp
      MLX/mlx/base.cpp
      MLX/mlx/linear.cpp
      MLX/mlx/positional_encoding.cpp
      MLX/mlx/activations.cpp
      MLX/mlx/embedding.cpp
      MLX/mlx/normalization.cpp
      MLX/mlx/transformer.cpp
      MLX/mlx/quantized.cpp
    )
    
    find_package(MLX CONFIG)
    if(MLX_FOUND)
      message(STATUS "Found MLX: ${MLX_INCLUDE_DIRS}")
    else()
      # Not support directly download from source 
      find_library(ACCELERATE_LIBRARY Accelerate)
      find_library(METAL_LIB Metal)
      find_library(FOUNDATION_LIB Foundation)
      find_library(QUARTZ_LIB QuartzCore)
      message(STATUS "Downloading MLX source")
      FetchContent_Declare(
        mlx
        GIT_REPOSITORY https://github.com/ml-explore/mlx.git
        GIT_TAG        v0.16.0
        GIT_SHALLOW    FALSE
      )
      set(MLX_BUILD_GGUF OFF)
      set(MLX_BUILD_TESTS OFF)
      set(MLX_BUILD_EXAMPLES OFF)
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

    target_include_directories(wasmedgePluginWasiNN
      PUBLIC
      ${tokenizers_SOURCE_DIR}/include
    )
    target_include_directories(wasmedgePluginWasiNN
      SYSTEM PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/MLX
      ${MLX_INCLUDE_DIRS}
      $<BUILD_INTERFACE:${gguflib_SOURCE_DIR}>
    )
    target_link_libraries(wasmedgePluginWasiNN
      PRIVATE
      tokenizers_cpp
      ${MLX_LIBRARIES}
      gguflib
      mlx
      simdjson::simdjson
    )
  else()
    # Add the dependency of other backends here.
    message(FATAL_ERROR "WASI-NN: backend ${BACKEND} not found or unimplemented.")
  endif()
endforeach()

target_compile_options(wasmedgePluginWasiNN
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginWasiNN
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_BUILD_WASI_NN_RPC)
  add_definitions(-DWASMEDGE_BUILD_WASI_NN_RPC)
  target_include_directories(wasmedgePluginWasiNN
    SYSTEM BEFORE PUBLIC ${Protobuf_INCLUDE_DIR}
  )
  target_link_libraries(wasmedgePluginWasiNN
    PRIVATE
    wasiNNRPC
  )
endif()

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasiNN
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasiNN
    PRIVATE
    wasmedge_shared
  )
endif()

install(
  TARGETS wasmedgePluginWasiNN
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
  COMPONENT WasmEdge
)
