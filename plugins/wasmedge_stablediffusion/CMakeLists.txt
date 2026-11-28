# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors


if(WASMEDGE_PLUGIN_STABLEDIFFUSION_CUDA)
  message(STATUS "Stable diffusion plugin: Enable SD_CUDA")
  set(SD_CUDA ON CACHE INTERNAL "Stable diffusion plugin: Enable SD_CUDA")
  set(SD_CUDA_CUTLASS_PATH "" CACHE INTERNAL "Stable diffusion plugin: Build without CUTLASS")
else()
  message(STATUS "Stable diffusion plugin: Disable SD_CUDA")
  set(SD_CUDA OFF CACHE INTERNAL "Stable diffusion plugin: Disable SD_CUDA")
  unset(SD_CUDA_CUTLASS_PATH CACHE)
endif()

# SD_METAL does not control the Metal backend of ggml. Set GGML_METAL to the
# same value.
if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" AND WASMEDGE_PLUGIN_STABLEDIFFUSION_METAL)
  message(STATUS "Stable diffusion plugin: Enable SD_METAL and GGML_METAL")
  set(SD_METAL ON CACHE INTERNAL "Stable diffusion plugin: Enable SD_METAL")
  set(GGML_METAL ON CACHE INTERNAL "Stable diffusion plugin: Enable GGML_METAL")
  set(GGML_METAL_EMBED_LIBRARY ON)
else()
  message(STATUS "Stable diffusion plugin: Disable SD_METAL and GGML_METAL")
  set(SD_METAL OFF CACHE INTERNAL "Stable diffusion plugin: Disable SD_METAL")
  set(GGML_METAL OFF CACHE INTERNAL "Stable diffusion plugin: Disable GGML_METAL")
endif()

# The GPU builds use the CUDA and Metal backends of ggml, not the library's own
# ggml extensions:
# - The library's CUDA kernels need toolkit 12.2 or later. The CI images have
#   11.3 and 12.0.
# - The library's Metal kernels enable Objective-C++ for the whole project. Then
#   lib/host/wasi/macos.mm has no compile rule.
# ABI.md, section 6.5, gives the effect on models. The switch also changes the
# CPU code of the library, so the CPU builds, which CI tests, keep the
# extensions.
if(SD_CUDA OR SD_METAL)
  message(STATUS "Stable diffusion plugin: Enable SD_USE_UPSTREAM_GGML")
  set(SD_USE_UPSTREAM_GGML ON CACHE INTERNAL "Stable diffusion plugin: Build ggml without the library's own extensions")
else()
  message(STATUS "Stable diffusion plugin: Disable SD_USE_UPSTREAM_GGML")
  set(SD_USE_UPSTREAM_GGML OFF CACHE INTERNAL "Stable diffusion plugin: Build ggml with the library's own extensions")
endif()

if(WASMEDGE_PLUGIN_STABLEDIFFUSION_OPENMP)
  message(STATUS "Stable diffusion plugin: Enable SD_OPENMP")
  set(GGML_OPENMP ON)
else()
  message(STATUS "Stable diffusion plugin: Disable SD_OPENMP")
  set(GGML_OPENMP OFF)
endif()

# Build ggml for fixed CPU features, not for the CPU of the build machine.
# Every x86_64 switch is set: the cache and the defaults of ggml do not apply.
set(GGML_NATIVE OFF)
set(GGML_SSE42 ON)
set(GGML_AVX ON)
set(GGML_AVX2 ON)
set(GGML_BMI2 ON)
set(GGML_FMA ON)
set(GGML_F16C ON)
set(GGML_AVX_VNNI OFF)
set(GGML_AVX512 OFF)
set(GGML_AVX512_VBMI OFF)
set(GGML_AVX512_VNNI OFF)
set(GGML_AVX512_BF16 OFF)
set(GGML_AMX_TILE OFF)
set(GGML_AMX_INT8 OFF)
set(GGML_AMX_BF16 OFF)

# The library is pinned to one revision and linked statically. The plug-in is
# then one file that needs only libwasmedge and system libraries (plus the CUDA
# libraries in a CUDA build). The archives go into a shared object, so they need
# position-independent code. Read DESIGN.md before you move the pin.
message(STATUS "Downloading stable diffusion source")
FetchContent_Declare(
  stable-diffusion
  GIT_REPOSITORY https://github.com/leejet/stable-diffusion.cpp.git
  GIT_TAG        8187e47c4ab3b12af231cc689a41749a5e3c6a16
  GIT_SHALLOW    TRUE
  GIT_SUBMODULES ggml
  )
set(SD_BUILD_SHARED_LIBS OFF CACHE INTERNAL "Stable diffusion plugin: Build static libs")
set(SD_BUILD_EXAMPLES OFF CACHE INTERNAL "Stable diffusion plugin: Do not build examples")
set(SD_WEBP OFF CACHE INTERNAL "Stable diffusion plugin: Disable WebP")
set(SD_WEBM OFF CACHE INTERNAL "Stable diffusion plugin: Disable WebM")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
FetchContent_MakeAvailable(stable-diffusion)
if(APPLE AND CMAKE_SYSTEM_VERSION VERSION_LESS 23)
  # `cblas_sgemm()` introduced in macOS 13.3.
  set(GGML_NO_ACCELERATE ON CACHE INTERNAL "Stable diffusion plugin: Turn off accelerate")
endif()

# All sources except the two that need the library. The plug-in and the test
# binary link this core. The tests use a fake backend and a scripted library.
wasmedge_add_library(wasmedgePluginWasmEdgeStableDiffusionCore
  STATIC
  sd_adapter_check.cpp
  sd_backend_sdcpp.cpp
  sd_func.cpp
  sd_guest_file.cpp
  sd_image.cpp
  sd_module.cpp
  sd_request.cpp
  sd_session.cpp
)

# sd_image.cpp compiles the stb decoders. They overflow signed integers on
# crafted input. -fwrapv makes the overflow defined.
set_source_files_properties(sd_image.cpp
  PROPERTIES
  COMPILE_OPTIONS -fwrapv
)

target_compile_options(wasmedgePluginWasmEdgeStableDiffusionCore
  PUBLIC
  -DWASMEDGE_PLUGIN
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasmEdgeStableDiffusionCore
    PUBLIC
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasmEdgeStableDiffusionCore
    PUBLIC
    wasmedge_shared
  )
endif()

# Build simdjson from source and link it statically, so that every build has
# the same parser. A simdjson of the build host (Homebrew has one) is a shared
# library, and the plug-in would then need it at run time. simdjson takes its
# library type from BUILD_SHARED_LIBS, so that variable is off here.
set(WASMEDGE_FORCE_DOWNLOAD_SIMDJSON ON)
set(BUILD_SHARED_LIBS OFF)
wasmedge_setup_simdjson()
target_link_libraries(wasmedgePluginWasmEdgeStableDiffusionCore
  PRIVATE
  simdjson::simdjson
)

# The headers of that simdjson come first. Another package of the build host
# can bring the headers of the host's simdjson into the include path.
target_include_directories(wasmedgePluginWasmEdgeStableDiffusionCore
  SYSTEM BEFORE
  PRIVATE
  $<TARGET_PROPERTY:simdjson::simdjson,INTERFACE_INCLUDE_DIRECTORIES>
)

target_include_directories(wasmedgePluginWasmEdgeStableDiffusionCore
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# The library header, plus stb_image.h and stb_image_write.h from the library
# tree. WasmEdge has no copy of stb.
target_include_directories(wasmedgePluginWasmEdgeStableDiffusionCore
  SYSTEM
  PRIVATE
  "${stable-diffusion_SOURCE_DIR}/include"
  "${stable-diffusion_SOURCE_DIR}/thirdparty"
)


wasmedge_add_library(wasmedgePluginWasmEdgeStableDiffusion
  SHARED
  sd_backend_sdcpp_bind.cpp
  sd_env.cpp
)

target_link_libraries(wasmedgePluginWasmEdgeStableDiffusion
  PRIVATE
  wasmedgePluginWasmEdgeStableDiffusionCore
  stable-diffusion
  ${CMAKE_THREAD_LIBS_INIT}
)

# Export no symbol of the archives. Then the plug-in offers no ggml symbol to
# the process, and a second ggml (the WASI-NN GGML plug-in) cannot replace its
# copy. macOS uses two-level namespaces and needs no option.
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_link_options(wasmedgePluginWasmEdgeStableDiffusion
    PRIVATE
    -Wl,--exclude-libs,ALL
  )
endif()

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
    -Wno-deprecated-literal-operator
    -Wno-inconsistent-missing-override
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
