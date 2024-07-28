# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

# setup stable diffusion
message(STATUS "Downloading stable diffusion source")
FetchContent_Declare(
  stable-diffusion
  GIT_REPOSITORY https://github.com/leejet/stable-diffusion.cpp.git
  GIT_TAG        master-9c51d87
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(stable-diffusion)
set_property(TARGET stable-diffusion PROPERTY POSITION_INDEPENDENT_CODE ON)
if(APPLE AND CMAKE_SYSTEM_VERSION VERSION_LESS 23)
  # `cblas_sgemm()` introduced in macOS 13.3.
  set(GGML_NO_ACCELERATE ON CACHE INTERNAL "Stable diffusion turn off accelerate")
endif()
get_target_property(SD_DEPS stable-diffusion LINK_LIBRARIES)
foreach(dep ${SD_DEPS})
  if(TARGET ${dep})
    set_target_properties(${dep} PROPERTIES
      POSITION_INDEPENDENT_CODE ON
    )
  endif()
endforeach()

wasmedge_add_library(wasmedgePluginWasmEdgeStableDiffusion
  SHARED
  sd_env.cpp
  sd_func.cpp
  sd_module.cpp
)

target_link_libraries(wasmedgePluginWasmEdgeStableDiffusion PRIVATE stable-diffusion ${CMAKE_THREAD_LIBS_INIT})

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

if (MSVC)
  target_compile_options(
    wasmedgePluginWasmEdgeStableDiffusion
    PRIVATE
    /wd4459
    /wd4100
    /wd4127
    /wd4701
  )
else()
  target_compile_options(
    wasmedgePluginWasmEdgeStableDiffusion
    PRIVATE
		-Wno-unused-function
		-Wno-unused-variable
		-Wno-unused-parameter
		-Wno-missing-field-initializers
    -Wno-deprecated-declarations
	)
endif()

install(TARGETS wasmedgePluginWasmEdgeStableDiffusion DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
