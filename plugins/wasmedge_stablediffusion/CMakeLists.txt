# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC


if(WASMEDGE_PLUGIN_STABLEDIFFUSION_CUDA)
  message(STATUS "Stable diffusion plugin: Enable SD_CUDA")
  set(SD_CUDA ON CACHE BOOL "Stable diffusion plugin: Enable SD_CUDA")
else()
  message(STATUS "Stable diffusion plugin: Disable SD_CUDA")
  set(SD_CUDA OFF CACHE BOOL "Stable diffusion plugin: Disable SD_CUDA")
endif()

if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" AND WASMEDGE_PLUGIN_STABLEDIFFUSION_METAL)
  message(STATUS "Stable diffusion plugin: Enable SD_METAL")
  set(SD_METAL ON CACHE BOOL "Stable diffusion plugin: Enable SD_METAL")
  set(GGML_METAL_EMBED_LIBRARY ON)
else()
  message(STATUS "Stable diffusion plugin: Disable SD_METAL")
  set(SD_METAL OFF CACHE BOOL "Stable diffusion plugin: Disable SD_METAL")
endif()

if(WASMEDGE_PLUGIN_STABLEDIFFUSION_OPENMP)
  message(STATUS "Stable diffusion plugin: Enable SD_OPENMP")
  set(GGML_OPENMP ON)
else()
  message(STATUS "Stable diffusion plugin: Disable SD_OPENMP")
  set(GGML_OPENMP OFF)
endif()

# setup stable diffusion
message(STATUS "Downloading stable diffusion source")
FetchContent_Declare(
  stable-diffusion
  GIT_REPOSITORY https://github.com/leejet/stable-diffusion.cpp.git
  GIT_TAG        dcf91f9e0f2cbf9da472ee2a556751ed4bab2d2a
  GIT_SHALLOW    TRUE
  )
set(SD_BUILD_SHARED_LIBS ON CACHE INTERNAL "Stable diffusion plugin: Build shared libs")
FetchContent_MakeAvailable(stable-diffusion)
set_property(TARGET stable-diffusion PROPERTY POSITION_INDEPENDENT_CODE ON)
if(APPLE AND CMAKE_SYSTEM_VERSION VERSION_LESS 23)
  # `cblas_sgemm()` introduced in macOS 13.3.
  set(GGML_NO_ACCELERATE ON CACHE INTERNAL "Stable diffusion plugin: Turn off accelerate")
endif()


wasmedge_add_library(wasmedgePluginWasmEdgeStableDiffusion
  SHARED
  sd_env.cpp
  sd_func.cpp
  sd_module.cpp
)

target_link_libraries(wasmedgePluginWasmEdgeStableDiffusion
  PRIVATE
  stable-diffusion
  ${CMAKE_THREAD_LIBS_INIT}
)

target_compile_options(wasmedgePluginWasmEdgeStableDiffusion
  PUBLIC
  -DWASMEDGE_PLUGIN
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasmEdgeStableDiffusion
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasmEdgeStableDiffusion
    PRIVATE
    wasmedge_shared
  )
endif()

target_include_directories(wasmedgePluginWasmEdgeStableDiffusion
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(wasmedgePluginWasmEdgeStableDiffusion
  SYSTEM
  PRIVATE
  "${stable-diffusion_SOURCE_DIR}/thirdparty"
)

if (MSVC)
  target_compile_options(
    stable-diffusion
    PRIVATE
    /wd4459
    /wd4100
    /wd4127
    /wd4701
  )
else()
  target_compile_options(
    stable-diffusion
    PRIVATE
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-deprecated-declarations
    -Wno-braced-scalar-init
    -Wno-unused-value
    -Wno-uninitialized
    -Wno-format
    -Wno-enum-compare
  )
endif()

install(
  TARGETS wasmedgePluginWasmEdgeStableDiffusion
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
  COMPONENT WasmEdge
)
