# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgePluginWasiNN
  SHARED
  wasinnenv.cpp
  wasinnfunc.cpp
  wasinnmodule.cpp
  openvino.cpp
  onnx.cpp
  tf.cpp
  torch.cpp
  tfl.cpp
  ggml.cpp
  neuralspeed.cpp
  piper.cpp
  whispercpp.cpp
  chattts.cpp
)

foreach(BACKEND ${WASMEDGE_PLUGIN_WASI_NN_BACKEND})
  string(TOLOWER ${BACKEND} BACKEND)
  if(BACKEND STREQUAL "ggml")
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
    target_link_libraries(wasmedgePluginWasiNN PRIVATE
      common
      simdjson::simdjson
      llava
    )
    if(APPLE AND WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_METAL)
      add_custom_command(
        TARGET wasmedgePluginWasiNN
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${llama_SOURCE_DIR}/ggml/src/ggml-metal.metal ggml-metal.metal
        COMMAND ${CMAKE_COMMAND} -E copy ${llama_SOURCE_DIR}/ggml/src/ggml-common.h ggml-common.h
      )
    endif()
  elseif(BACKEND STREQUAL "neuralspeed")
    message(NOTICE "WASI-NN NeuralSpeed backend is removed due to the upstream end-of-life.")
    message(NOTICE "Reference: https://github.com/intel/neural-speed")
  elseif(BACKEND STREQUAL "chattts")
    wasmedge_setup_simdjson()

    find_package(Python3 COMPONENTS Interpreter Development)
    if(Python3_FOUND)
      target_compile_definitions(wasmedgePluginWasiNN
        PRIVATE PYTHON_LIB_PATH="${Python3_LIBRARIES}"
      )
      include_directories(${Python3_INCLUDE_DIRS})
      target_link_libraries(wasmedgePluginWasiNN PRIVATE ${Python3_LIBRARIES})
      target_link_directories(wasmedgePluginWasiNN PRIVATE ${Python3_RUNTIME_LIBRARY_DIRS})
    else()
      message(FATAL_ERROR "Can not find python3.")
    endif()
    target_link_libraries(wasmedgePluginWasiNN PRIVATE simdjson::simdjson)
  elseif(BACKEND STREQUAL "piper")
    wasmedge_setup_simdjson()
    target_link_libraries(wasmedgePluginWasiNN PRIVATE simdjson::simdjson)
  elseif(BACKEND STREQUAL "whisper")
    wasmedge_setup_simdjson()
    if(APPLE AND CMAKE_SYSTEM_VERSION VERSION_LESS 23)
      # `cblas_sgemm()` introduced in macOS 13.3.
      set(WHISPER_NO_ACCELERATE ON CACHE INTERNAL "Stable diffusion turn off accelerate")
    endif()
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Whisper not build shared")
    include(FetchContent)
    FetchContent_Declare(
      whisper
      GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
      GIT_TAG        v1.6.2
      GIT_SHALLOW    FALSE
    )
    FetchContent_MakeAvailable(whisper)
    set_property(TARGET whisper PROPERTY POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(wasmedgePluginWasiNN PRIVATE
      whisper
      simdjson::simdjson
    )
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

include(WASINNDeps)
wasmedge_setup_wasinn_target(wasmedgePluginWasiNN)

install(
  TARGETS wasmedgePluginWasiNN
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
