# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

# llama.cpp options
# Disable warnings and debug messages
set(LLAMA_ALL_WARNINGS OFF)
set(LLAMA_METAL_NDEBUG ON)

if(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_CUBLAS)
  message(STATUS "WASI-NN GGML LLAMA backend: Enable LLAMA_CUBLAS")
  # Default use OpenBLAS
  set(LLAMA_CUBLAS ON)
else()
  message(STATUS "WASI-NN GGML LLAMA backend: Disable LLAMA_CUBLAS")
  set(LLAMA_CUBLAS OFF)
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
else()
  message(STATUS "WASI-NN GGML LLAMA backend: Disable LLAMA_METAL")
  set(LLAMA_METAL OFF)
endif()

# simdjson for ggml backend
find_package(simdjson QUIET)
if(simdjson_FOUND)
  message(STATUS "SIMDJSON found")
else()
  message(STATUS "Downloading SIMDJSON source")
  include(FetchContent)
  FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG  tags/v3.2.1
    GIT_SHALLOW TRUE)

  if(MSVC)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      get_property(
        compile_options
        DIRECTORY
        PROPERTY COMPILE_OPTIONS
        )
      set_property(
        DIRECTORY
        APPEND
        PROPERTY COMPILE_OPTIONS
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
      unset(compile_options)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
      set_property(
        DIRECTORY
        APPEND
        PROPERTY COMPILE_OPTIONS
        /wd4100 # unreferenced formal parameter
        )
    endif()
  endif()

  FetchContent_MakeAvailable(simdjson)
  set_property(TARGET simdjson PROPERTY POSITION_INDEPENDENT_CODE ON)

  message(STATUS "Downloading SIMDJSON source -- done")
endif()

add_subdirectory(thirdparty)

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

target_include_directories(wasmedgePluginWasiNN
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/thirdparty/ggml
)

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

string(TOLOWER ${WASMEDGE_PLUGIN_WASI_NN_BACKEND} BACKEND)
if(BACKEND STREQUAL "ggml")
  target_link_libraries(wasmedgePluginWasiNN PRIVATE llama simdjson)
endif()

include(WASINNDeps)
wasmedge_setup_wasinn_target(wasmedgePluginWasiNN)

install(TARGETS wasmedgePluginWasiNN DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
