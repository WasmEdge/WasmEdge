# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

string(TOLOWER ${WASMEDGE_PLUGIN_WASI_NN_BACKEND} BACKEND)
if(BACKEND STREQUAL "ggml")
  # llama.cpp options
  # Disable warnings and debug messages
  set(LLAMA_ALL_WARNINGS OFF)
  set(LLAMA_METAL_NDEBUG ON)
  set(LLAMA_ACCELERATE OFF)

  if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_NATIVE)
    message(STATUS "WASI-NN GGML LLAMA backend: Enable LLAMA_NATIVE(AVX/AVX2/FMA)")
    set(LLAMA_NATIVE ON)
  else()
    set(LLAMA_NATIVE OFF)
  endif()

  if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_CUBLAS)
    message(STATUS "WASI-NN GGML LLAMA backend: Enable LLAMA_CUDA")
    set(LLAMA_CUDA ON)
    # We need to set GGML_USE_CUDA for clip from llava.
    add_compile_definitions(GGML_USE_CUDA)
    # If CUDA is ON, then OpenBLAS should be OFF.
    set(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_BLAS OFF)
  else()
    message(STATUS "WASI-NN GGML LLAMA backend: Disable LLAMA_CUDA")
    set(LLAMA_CUDA OFF)
  endif()

  if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_BLAS)
    message(STATUS "WASI-NN GGML LLAMA backend: Enable LLAMA_BLAS")
    # Default use OpenBLAS
    set(LLAMA_BLAS ON)
    set(LLAMA_BLAS_VENDOR "OpenBLAS")
  else()
    message(STATUS "WASI-NN GGML LLAMA backend: Disable LLAMA_BLAS")
    set(LLAMA_BLAS OFF)
  endif()

  if(NOT APPLE)
    set(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_METAL OFF)
  endif()

  if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_METAL)
    message(STATUS "WASI-NN GGML LLAMA backend: Enable LLAMA_METAL")
    set(LLAMA_METAL ON)
    set(LLAMA_METAL_EMBED_LIBRARY ON)
  else()
    message(STATUS "WASI-NN GGML LLAMA backend: Disable LLAMA_METAL")
    set(LLAMA_METAL OFF)
  endif()

  # setup llama.cpp
  message(STATUS "Downloading llama.cpp source")
  if(MSVC)
    add_compile_options(
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
  endif()
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(
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
    add_compile_options(
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
    )
  endif()
  include(FetchContent)
  FetchContent_Declare(
    llama
    GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
    GIT_TAG        b2943
    GIT_SHALLOW    FALSE
  )
  FetchContent_MakeAvailable(llama)
  set_property(TARGET ggml PROPERTY POSITION_INDEPENDENT_CODE ON)
  set_property(TARGET common PROPERTY POSITION_INDEPENDENT_CODE ON)
  set_property(TARGET llama PROPERTY POSITION_INDEPENDENT_CODE ON)

  # setup simdjson
  find_package(simdjson QUIET)
  if(simdjson_FOUND)
    message(STATUS "SIMDJSON found")
  else()
    message(STATUS "Downloading SIMDJSON source")
    include(FetchContent)
    FetchContent_Declare(
      simdjson
      GIT_REPOSITORY https://github.com/simdjson/simdjson.git
      GIT_TAG  tags/v3.9.1
      GIT_SHALLOW TRUE)

    if(MSVC)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(
          -Wno-undef
          -Wno-suggest-override
          -Wno-documentation
          -Wno-sign-conversion
          -Wno-extra-semi-stmt
          -Wno-old-style-cast
          -Wno-error=unused-parameter
          -Wno-error=unused-template
          -Wno-conditional-uninitialized
          -Wno-implicit-int-conversion
          -Wno-shorten-64-to-32
          -Wno-range-loop-bind-reference
          -Wno-format-nonliteral
          -Wno-unused-exception-parameter
          -Wno-unused-member-function
        )
      elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        add_compile_options(
          /wd4100 # unreferenced formal parameter
        )
      endif()
    endif()

    FetchContent_MakeAvailable(simdjson)
    set_property(TARGET simdjson PROPERTY POSITION_INDEPENDENT_CODE ON)

    message(STATUS "Downloading SIMDJSON source -- done")
  endif()
endif()

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
)

target_compile_options(wasmedgePluginWasiNN
  PUBLIC
  -DWASMEDGE_PLUGIN
)
if(WASMEDGE_BUILD_WASI_NN_RPC)
  add_definitions(-DWASMEDGE_BUILD_WASI_NN_RPC)
endif()

target_include_directories(wasmedgePluginWasiNN
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(BACKEND STREQUAL "ggml")
  # Setup llava from llama.cpp
  wasmedge_add_library(llava OBJECT
    ${llama_SOURCE_DIR}/examples/llava/clip.cpp
    ${llama_SOURCE_DIR}/examples/llava/llava.cpp
  )
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(llava PRIVATE -Wno-error=unused-variable -Wno-error=unused-function)
  endif()
  target_link_libraries(llava PRIVATE ggml llama)
  target_include_directories(llava PUBLIC
    ${llama_SOURCE_DIR}
    ${llama_SOURCE_DIR}/common
    ${llama_SOURCE_DIR}/examples/llava
  )
  # Setup include and link from llama.cpp
  target_include_directories(wasmedgePluginWasiNN PRIVATE
    ${llama_SOURCE_DIR}
    ${llama_SOURCE_DIR}examples/llava
  )
  target_link_libraries(wasmedgePluginWasiNN PRIVATE
    common
    simdjson::simdjson
    llava
  )
  if(MSVC)
    target_compile_options(wasmedgePluginWasiNN PUBLIC
      /wd4067 # unexpected tokens following preprocessor directive - expected a newline
    )
  endif()
  if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_METAL)
    add_custom_command(
      TARGET wasmedgePluginWasiNN
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${llama_SOURCE_DIR}/ggml-metal.metal ggml-metal.metal
      COMMAND ${CMAKE_COMMAND} -E copy ${llama_SOURCE_DIR}/ggml-common.h ggml-common.h
    )
  endif()
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

if(WASMEDGE_BUILD_WASI_NN_RPC)
  target_include_directories(wasmedgePluginWasiNN
    SYSTEM BEFORE PUBLIC ${Protobuf_INCLUDE_DIR}
  )
  target_link_libraries(wasmedgePluginWasiNN
    PRIVATE
    wasiNNRPC
  )
endif()

include(WASINNDeps)
wasmedge_setup_wasinn_target(wasmedgePluginWasiNN)

install(TARGETS wasmedgePluginWasiNN DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
