# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

include_guard()

include(FetchContent)

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
      GIT_SHALLOW    FALSE
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
