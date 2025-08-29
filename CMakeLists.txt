# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

cmake_minimum_required(VERSION 3.18)
cmake_policy(SET CMP0091 NEW)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()
project(WasmEdge LANGUAGES CXX C)

# CMake build type.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Overwrite it if you want to use static MSVC runtime library.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

# WasmEdge CAPI and so versions.
set(WASMEDGE_CAPI_VERSION "0.1.0" CACHE STRING "WasmEdge C API library version")
set(WASMEDGE_CAPI_SOVERSION "0" CACHE STRING "WasmEdge C API library soversion")

# Set cpack package version.
find_program(GIT_CMD git)
# Assuming the git command is not found and .git folder is not available.
set(GIT_VERSION_NOT_FOUND 1)
if(GIT_CMD AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  execute_process(COMMAND
    ${GIT_CMD} describe --match "[0-9].[0-9]*" --tag
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE CPACK_PACKAGE_VERSION
    RESULT_VARIABLE GIT_VERSION_NOT_FOUND
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()
if(GIT_VERSION_NOT_FOUND AND NOT GIT_VERSION_NOT_FOUND EQUAL 0)
  set(CPACK_PACKAGE_VERSION "0.0.0-unreleased")
endif()

# Overwrite version information once there is a VERSION file.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" LOCAL_VERSION)
  set(CPACK_PACKAGE_VERSION ${LOCAL_VERSION})
  unset(LOCAL_VERSION)
endif()

# Add the CMake module path.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Export compile commands.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find threads and filesystem.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Filesystem REQUIRED Final Experimental)
find_package(Threads REQUIRED)

# List of WasmEdge options
option(WASMEDGE_BUILD_TESTS "Generate build targets for the wasmedge unit tests." OFF)
option(WASMEDGE_BUILD_COVERAGE "Generate coverage report. Require WASMEDGE_BUILD_TESTS." OFF)
option(WASMEDGE_BUILD_SHARED_LIB "Generate the WasmEdge shared library." ON)
option(WASMEDGE_BUILD_STATIC_LIB "Generate the WasmEdge static library." OFF)
option(WASMEDGE_BUILD_TOOLS "Generate wasmedge and wasmedgec tools. Depend on and will build the WasmEdge shared library." ON)
option(WASMEDGE_BUILD_FUZZING "Generate fuzzing test tools. Couldn't build with wasmedge tools and unit tests." OFF)
option(WASMEDGE_BUILD_PLUGINS "Generate plugins." ON)
option(WASMEDGE_BUILD_EXAMPLE "Generate examples." OFF)
option(WASMEDGE_BUILD_WASI_NN_RPC "Generate WASI-NN RPC." OFF)
option(WASMEDGE_USE_LLVM "Enable WasmEdge LLVM-based compilation runtime." ON)
if(WASMEDGE_BUILD_AOT_RUNTIME)
  message(WARNING "WASMEDGE_BUILD_AOT_RUNTIME option was renamed to WASMEDGE_USE_LLVM.")
  set(WASMEDGE_USE_LLVM "${WASMEDGE_BUILD_AOT_RUNTIME}" CACHE STRING "Enable WasmEdge LLVM-based compilation runtime.")
  unset(WASMEDGE_BUILD_AOT_RUNTIME CACHE)
endif()
option(WASMEDGE_USE_CXX11_ABI "Enable cxx11 abi when building WasmEdge." ON)
option(WASMEDGE_FORCE_DISABLE_LTO "Forcefully disable link time optimization when linking even in Release/RelWithDeb build." OFF)
option(WASMEDGE_LINK_LLVM_STATIC "Statically link the LLVM library into the WasmEdge tools and libraries." OFF)
option(WASMEDGE_LINK_TOOLS_STATIC "Statically link the wasmedge and wasmedgec tools. Will forcefully link the LLVM library statically." OFF)
option(WASMEDGE_ENABLE_UB_SANITIZER "Enable undefined behavior sanitizer." OFF)
option(WASMEDGE_DISABLE_LIBTINFO "Disable linking against libtinfo when linking LLVM." OFF)

# Options about plug-ins.
#   WASI plug-in: WASI-Crypto proposal.
option(WASMEDGE_PLUGIN_WASI_CRYPTO "Enable and build WasmEdge wasi-crypto plugin." OFF)
#   WASI plug-in: WASI-Http proposal.
option(WASMEDGE_PLUGIN_WASI_HTTP "Enable and build WasmEdge wasi-http plugin." OFF)
#   WASI plug-in: WASI-Logging proposal.
#     Note: WASMEDGE_PLUGIN_WASI_LOGGING is not used until the new plug-in mechanism ready in 0.15.0.
option(WASMEDGE_PLUGIN_WASI_LOGGING "Enable and build WasmEdge wasi-logging plugin." ON)
#   WASI plug-in: WASI-NN proposal with backends.
set(WASMEDGE_PLUGIN_WASI_NN_BACKEND "" CACHE STRING "Enable and build WasmEdge Wasi-NN plugin with backends.")
option(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_NATIVE "Enable LLAMA_NATIVE(AVX/AVX2/FMA) in the WASI-NN GGML backend." ON)
option(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_BLAS "Enable LLAMA_BLAS in the WASI-NN GGML backend." OFF)
option(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_CUBLAS "Enable LLAMA_CUBLAS in the WASI-NN GGML backend." OFF)
option(WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_METAL "Enable LLAMA_METAL in the WASI-NN GGML backend. Available on MacOS arm64 only." ON)
option(WASMEDGE_PLUGIN_WASI_NN_WHISPER_METAL "Enable GGML_METAL in the WASI-NN WHISPER backend. Available on MacOS arm64 only." ON)
option(WASMEDGE_PLUGIN_WASI_NN_WHISPER_CUDA "Enable GGML_CUDA in the WASI-NN WHISPER backend." OFF)
option(WASMEDGE_PLUGIN_WASI_NN_BITNET_ARM_TL1 "Enable BitNet.cpp ARM_TL1 optimization" OFF)
option(WASMEDGE_PLUGIN_WASI_NN_BITNET_X86_TL2 "Enable BitNet.cpp X86_TL2 optimization" OFF)
#   WASI plug-in: WASI-Poll proposal.
option(WASMEDGE_PLUGIN_WASI_POLL "Enable and build WasmEdge wasi-poll plugin." OFF)
#   WasmEdge plug-in: wasm-bpf.
option(WASMEDGE_PLUGIN_WASM_BPF "Enable and build WasmEdge wasm-bpf plugin." OFF)
#   WasmEdge plug-in: ffmpeg.
option(WASMEDGE_PLUGIN_FFMPEG "Enable and build WasmEdge ffmpeg plugin." OFF)
#   WasmEdge plug-in: Image.
option(WASMEDGE_PLUGIN_IMAGE "Enable and build WasmEdge image plugin." OFF)
#   WasmEdge plug-in: LLMC.
option(WASMEDGE_PLUGIN_LLMC "Enable and build WasmEdge LLMC plugin." OFF)
#   WasmEdge plug-in: OCR.
option(WASMEDGE_PLUGIN_OCR "Enable and build WasmEdge OCR plugin." OFF)
#   WasmEdge plug-in: OpenCV-mini.
option(WASMEDGE_PLUGIN_OPENCVMINI "Enable and build WasmEdge opencvmini plugin." OFF)
#   WasmEdge plug-in: Process.
option(WASMEDGE_PLUGIN_PROCESS "Enable and build WasmEdge process plugin." OFF)
#   WasmEdge plug-in: Stable-diffusion.
option(WASMEDGE_PLUGIN_STABLEDIFFUSION "Enable and build WasmEdge stable-diffusion plugin." OFF)
option(WASMEDGE_PLUGIN_STABLEDIFFUSION_CUDA "Enable CUDA in the stable-diffusion plugin." OFF)
option(WASMEDGE_PLUGIN_STABLEDIFFUSION_METAL "Enable Metal in the stable-diffusion plugin." ON)
option(WASMEDGE_PLUGIN_STABLEDIFFUSION_OPENMP "Enable OpenMP in the stable-diffusion plugin." OFF)
#   WasmEdge plug-in: TensorFlow.
option(WASMEDGE_PLUGIN_TENSORFLOW "Enable and build WasmEdge TensorFlow plugin." OFF)
#   WasmEdge plug-in: TensorFlow-Lite.
option(WASMEDGE_PLUGIN_TENSORFLOWLITE "Enable and build WasmEdge TensorFlow-Lite plugin." OFF)
#   WasmEdge plug-in: zlib.
option(WASMEDGE_PLUGIN_ZLIB "Enable and build WasmEdge zlib plugin." OFF)
#   WasmEdge plug-in dependencies.
option(WASMEDGE_FORCE_DOWNLOAD_SIMDJSON "Force download and build simdjson for the wasi-nn plugin." OFF)
# Fuzzing and tools and tests are exclusive.
if(WASMEDGE_BUILD_TOOLS AND WASMEDGE_BUILD_FUZZING)
  message(FATAL_ERROR "wasmedge tool and fuzzing tool are exclusive options.")
endif()
if(WASMEDGE_BUILD_TESTS AND WASMEDGE_BUILD_FUZZING)
  message(FATAL_ERROR "unit tests and fuzzing tool are exclusive options.")
endif()

# Static library will forcefully turn off the LTO.
if(WASMEDGE_BUILD_STATIC_LIB)
  set(WASMEDGE_FORCE_DISABLE_LTO ON)
endif()

# WasmEdge tool options.
if(WASMEDGE_BUILD_TOOLS)
  if(WASMEDGE_LINK_TOOLS_STATIC)
    # Static tools will link LLVM statically.
    set(WASMEDGE_LINK_LLVM_STATIC ON)
    # Tools will forcefully turn on the static library building.
    set(WASMEDGE_BUILD_STATIC_LIB ON)
    if(WASMEDGE_BUILD_PLUGINS)
      message(WARNING "For tuning on the WASMEDGE_LINK_TOOLS_STATIC option, the plugins will not work.")
    endif()
  else()
    # Tools will forcefully turn on the shared library building.
    set(WASMEDGE_BUILD_SHARED_LIB ON)
  endif()
endif()

if(WASMEDGE_BUILD_WASI_NN_RPC)
  # * Homebrew: grpc
  # * Debian, Ubuntu: libgrpc-dev, libgrpc++-dev
  find_package(PkgConfig)
  if(PkgConfig_FOUND)
    pkg_check_modules(gRPCPP grpc++)
  endif()
  # Do not check find_package(gRPC), because libgrpc-dev for Ubuntu 22.04 does not contain cmake files.
  # https://packages.ubuntu.com/search?keywords=libgrpc-dev
  # Do not check find_package(protobuf), because libprotobuf-dev for Ubuntu does not contain cmake files.
  # https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=1027876
  if(gRPCPP_FOUND AND WASMEDGE_BUILD_SHARED_LIB)
    message(STATUS "If you see an error related to gRPC or protobuf, try setting WASMEDGE_BUILD_WASI_NN_RPC to OFF.")
  endif()
  if(WASMEDGE_BUILD_WASI_NN_RPC AND NOT WASMEDGE_BUILD_SHARED_LIB)
    message(FATAL_ERROR "WASMEDGE_BUILD_WASI_NN_RPC depends on WASMEDGE_BUILD_SHARED_LIB.")
  endif()
endif()

set(WASMEDGE_BUILD_PACKAGE "DEB;RPM" CACHE STRING "Package generate types")
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpack_config.cmake)

if(WASMEDGE_BUILD_COVERAGE)
  set(GCOVR_ADDITIONAL_ARGS "--exclude-unreachable-branches;--exclude-throw-branches")
  include(CodeCoverage)
  append_coverage_compiler_flags()
endif()

include(FetchContent)
include(Helper)
include(GNUInstallDirs)

set(CPACK_PACKAGE_VENDOR Second State LLC)
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_STRIP_FILES ON)
set(CPACK_PACKAGE_CONTACT "Shen-Ta Hsieh <beststeve@secondstate.io>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High performance WebAssembly Virtual Machine")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_ARCHIVE_WASMEDGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_RPM_MAIN_COMPONENT "WasmEdge")
set(CPACK_RPM_PACKAGE_LICENSE "Apache 2.0")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_WASMEDGE_PACKAGE_NAME wasmedge)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_COMPRESSION_TYPE xz)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/WasmEdge/WasmEdge/")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
set(CPACK_GENERATOR "${WASMEDGE_BUILD_PACKAGE}")
set(CPACK_PACKAGE_DESCRIPTION "WasmEdge is a high performance, extensible, and hardware optimized WebAssembly Virtual Machine for cloud, AI, and blockchain applications.")
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  # If WasmEdge is the top project, only pack the WasmEdge component.
  set(CPACK_COMPONENTS_ALL "WasmEdge")
endif()

add_subdirectory(include)
add_subdirectory(lib)
if(WASMEDGE_BUILD_PLUGINS AND (WASMEDGE_BUILD_STATIC_LIB OR WASMEDGE_BUILD_SHARED_LIB))
  # Plug-ins should depend on the WasmEdge library.
  if(WASMEDGE_BUILD_STATIC_LIB AND NOT WASMEDGE_BUILD_SHARED_LIB)
    # Link to the static library if only the WasmEdge static library is built.
    # If the WasmEdge shared library is built, the plug-ins will link to the shared library.
    set(WASMEDGE_LINK_PLUGINS_STATIC ON)
  endif()
  add_subdirectory(plugins)
endif()
add_subdirectory(thirdparty)
if(WASMEDGE_BUILD_TOOLS OR WASMEDGE_BUILD_FUZZING)
  add_subdirectory(tools)
endif()
if(WASMEDGE_BUILD_TESTS)
  include(CTest)
  add_subdirectory(test)
endif()
if(WASMEDGE_BUILD_EXAMPLE)
  add_subdirectory(examples/plugin/get-string)
endif()

include(CPack)
include(CPackComponent)
