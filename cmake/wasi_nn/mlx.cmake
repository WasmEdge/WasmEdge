# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

include_guard()

include(FetchContent)

# Function for preparing the MLX library.
function(wasmedge_setup_mlx_target target)
  wasmedge_setup_simdjson()
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
