# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgePluginWasiNN
  SHARED
  wasinnenv.cpp
  wasinnfunc.cpp
  wasinnmodule.cpp
)

target_compile_options(wasmedgePluginWasiNN
  PUBLIC
  -DWASMEDGE_PLUGIN
)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  target_link_options(wasmedgePluginWasiNN
    PUBLIC
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterC1EPKNS0_6Plugin16PluginDescriptorE
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterD1Ev
  )
endif()

target_include_directories(wasmedgePluginWasiNN
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
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

install(TARGETS wasmedgePluginWasiNN DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)

# Add backends building flags.
foreach(BACKEND ${WASMEDGE_PLUGIN_WASI_NN_BACKEND})
  string(TOLOWER ${BACKEND} BACKEND)
  if(BACKEND STREQUAL "openvino")
    message(STATUS "WASI-NN: Build OpenVINO backend for WASI-NN")
    find_package(InferenceEngine REQUIRED)
    add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO)
    target_link_libraries(wasmedgePluginWasiNN
      PUBLIC
      ${InferenceEngine_LIBRARIES}
    )
  elseif(BACKEND STREQUAL "pytorch")
    message(STATUS "WASI-NN: Build PyTorch backend for WASI-NN")
    find_package(Torch REQUIRED)
    add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH)
    target_link_libraries(wasmedgePluginWasiNN
      PUBLIC
      ${TORCH_LIBRARIES}
    )
  elseif(BACKEND STREQUAL "tensorflowlite")
    message(STATUS "WASI-NN: Build Tensorflow lite backend for WASI-NN")
    # TODO: Move these complicated steps into a helper cmake.
    add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE)

    if(NOT WASMEDGE_DEPS_VERSION)
      set(WASMEDGE_DEPS_VERSION "0.11.1")
    endif()

    # Clone required shared libraries
    if(ANDROID)
      if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(WASMEDGE_TENSORFLOW_SYSTEM_NAME "android_aarch64")
        set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH "a25dafad049cbc998c1f9682c57aec22b2fe5799eeffdd4ed19793a734cde8a4")
      elseif()
        message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
      endif()
    elseif(APPLE)
      if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
        set(WASMEDGE_TENSORFLOW_SYSTEM_NAME "darwin_x86_64")
        set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH "2593772df440a768e79d87e74a860378f46fb0b7d1e7805879ab2ec26a093b57")
      else()
        message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
      endif()
    elseif(UNIX)
      if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
        set(WASMEDGE_TENSORFLOW_SYSTEM_NAME "manylinux2014_x86_64")
        set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH "43b2a782efb58b047c6d33f64d7ac711b24426959f91287d910edb8937c11dea")
      elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(WASMEDGE_TENSORFLOW_SYSTEM_NAME "manylinux2014_aarch64")
        set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH "1f47dcd05f32907848253e0f4b0eb3a6276802dae41d2b7de61214b75ba02395")
      else()
        message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
      endif()
    else()
      message(FATAL_ERROR "Unsupported system: ${CMAKE_SYSTEM_NAME}")
    endif()

    include(FetchContent)

    # Fetch Tensorflow-lite library.
    FetchContent_Declare(
      wasmedgetensorflowdepslite
      URL "https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/${WASMEDGE_DEPS_VERSION}/WasmEdge-tensorflow-deps-TFLite-${WASMEDGE_DEPS_VERSION}-${WASMEDGE_TENSORFLOW_SYSTEM_NAME}.tar.gz"
      URL_HASH "SHA256=${WASMEDGE_TENSORFLOW_DEPS_TFLITE_HASH}"
    )
    FetchContent_GetProperties(wasmedgetensorflowdepslite)

    if(NOT wasmedgetensorflowdepslite_POPULATED)
      message(STATUS "Downloading dependency: libtensorflowlite")
      FetchContent_Populate(wasmedgetensorflowdepslite)
      message(STATUS "Downloading dependency: libtensorflowlite - done")
    endif()

    # Setup Tensorflow-lite library.
    if(APPLE)
      set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_LIB
        "${wasmedgetensorflowdepslite_SOURCE_DIR}/libtensorflowlite_c.dylib"
      )
    elseif(UNIX)
      set(WASMEDGE_TENSORFLOW_DEPS_TFLITE_LIB
        "${wasmedgetensorflowdepslite_SOURCE_DIR}/libtensorflowlite_c.so"
      )
    endif()

    include(FetchContent)
    FetchContent_Declare(
      wasmedge_tensorflow_deps
      GIT_REPOSITORY https://github.com/second-state/WasmEdge-tensorflow-deps.git
      GIT_TAG ${WASMEDGE_DEPS_VERSION}
    )
    FetchContent_GetProperties(wasmedge_tensorflow_deps)

    if(NOT wasmedge_tensorflow_deps_POPULATED)
      message(STATUS "Fetching WasmEdge-tensorflow-dep repository")
      FetchContent_Populate(wasmedge_tensorflow_deps)
      message(STATUS "Fetching WasmEdge-tensorflow-dep repository - done")
    endif()

    set(WASMEDGE_TENSORFLOW_DEPS_PATH ${wasmedge_tensorflow_deps_SOURCE_DIR})
    set(WASMEDGE_TENSORFLOW_DEPS_BIN_PATH ${CMAKE_CURRENT_BINARY_DIR}/utils/WasmEdge-tensorflow-deps)

    message(STATUS "WASI-NN: Set WasmEdge-tensorflow deps source path: ${WASMEDGE_TENSORFLOW_DEPS_PATH}")
    message(STATUS "WASI-NN: Set WasmEdge-tensorflow deps binary path: ${WASMEDGE_TENSORFLOW_DEPS_BIN_PATH}")
    message(STATUS "WASI-NN: Set WasmEdge-tensorflowlite share path: ${WASMEDGE_TENSORFLOW_DEPS_TFLITE_LIB}")
    add_subdirectory(${WASMEDGE_TENSORFLOW_DEPS_PATH} ${WASMEDGE_TENSORFLOW_DEPS_BIN_PATH})
    target_include_directories(wasmedgePluginWasiNN
      PUBLIC
      ${TENSORFLOW_INCLUDE}
    )
    target_link_libraries(wasmedgePluginWasiNN
      PUBLIC
      ${WASMEDGE_TENSORFLOW_DEPS_TFLITE_LIB}
    )
  else()
    # Add the other backends here.
    message(FATAL_ERROR "WASI-NN: backend ${BACKEND} not found or unimplemented.")
  endif()
endforeach()
