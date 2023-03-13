# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

cmake_minimum_required(VERSION 3.10.2)
cmake_policy(SET CMP0091 NEW)
project(WasmEdge)

set(WASMEDGE_CAPI_VERSION "0.0.2" CACHE STRING "WasmEdge C API library version")
set(WASMEDGE_CAPI_SOVERSION "0" CACHE STRING "WasmEdge C API library soversion")

find_program(GIT_CMD git)
execute_process(COMMAND
  ${GIT_CMD} describe --match "[0-9].[0-9]*" --tag
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE CPACK_PACKAGE_VERSION
  RESULT_VARIABLE GIT_VERSION_NOT_FOUND
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(GIT_VERSION_NOT_FOUND AND NOT GIT_VERSION_NOT_FOUND EQUAL 0)
  set(CPACK_PACKAGE_VERSION "0.0.0-unreleased")
endif()

# Overwrite version information once there is a VERSION file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" LOCAL_VERSION)
  set(CPACK_PACKAGE_VERSION ${LOCAL_VERSION})
  unset(LOCAL_VERSION)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

include(FetchContent)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Filesystem REQUIRED Final Experimental)
find_package(Threads REQUIRED)

find_package(Boost QUIET)
if(${Boost_FOUND})
else()
  FetchContent_Declare(
    Boost
    URL https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.bz2
    URL_HASH SHA256=f0397ba6e982c4450f27bf32a2a83292aba035b827a5623a14636ea583318c41
  )
  set(BOOST_ENABLE_CMAKE ON)
  set(BOOST_RUNTIME_LINK static)
  set(BUILD_TESTING OFF)
  FetchContent_MakeAvailable(Boost)
  add_library(Boost_boost INTERFACE)
  add_library(Boost::boost ALIAS Boost_boost)
  target_include_directories(Boost_boost INTERFACE ${boost_SOURCE_DIR})
endif()

# List of WasmEdge options
option(WASMEDGE_BUILD_TESTS "Generate build targets for the wasmedge unit tests." OFF)
option(WASMEDGE_BUILD_COVERAGE "Generate coverage report. Require WASMEDGE_BUILD_TESTS." OFF)
option(WASMEDGE_BUILD_AOT_RUNTIME "Enable WasmEdge LLVM-based ahead of time compilation runtime." ON)
option(WASMEDGE_BUILD_SHARED_LIB "Generate the WasmEdge shared library." ON)
option(WASMEDGE_BUILD_STATIC_LIB "Generate the WasmEdge static library." OFF)
option(WASMEDGE_BUILD_TOOLS "Generate wasmedge and wasmedgec tools. Depend on and will build the WasmEdge shared library." ON)
option(WASMEDGE_BUILD_FUZZING "Generate fuzzing test tools. Couldn't build with wasmedge tools and unit tests." OFF)
option(WASMEDGE_BUILD_PLUGINS "Generate plugins." ON)
option(WASMEDGE_BUILD_EXAMPLE "Generate examples." OFF)
option(WASMEDGE_FORCE_DISABLE_LTO "Forcefully disable link time optimization when linking even in Release/RelWithDeb build." OFF)
option(WASMEDGE_LINK_LLVM_STATIC "Statically link the LLVM library into the WasmEdge tools and libraries." OFF)
option(WASMEDGE_LINK_TOOLS_STATIC "Statically link the wasmedge and wasmedgec tools. Will forcefully link the LLVM library statically." OFF)
set(WASMEDGE_PLUGIN_WASI_NN_BACKEND "" CACHE STRING "Enable WasmEdge Wasi-NN plugin with backends.")
# Currently supported WASI-NN backend: "OpenVINO" on Linux x86_64
option(WASMEDGE_PLUGIN_WASI_CRYPTO "Enable WasmEdge Wasi-crypto plugin." OFF)
option(WASMEDGE_PLUGIN_PROCESS "Enable WasmEdge process plugin." OFF)
option(WASMEDGE_PLUGIN_HTTPSREQ "Enable WasmEdge httpsreq plugin." OFF)

if(WASMEDGE_BUILD_TOOLS AND WASMEDGE_BUILD_FUZZING)
  message(FATAL_ERROR "wasmedge tool and fuzzing tool are exclusive options.")
endif()
if(WASMEDGE_BUILD_TESTS AND WASMEDGE_BUILD_FUZZING)
  message(FATAL_ERROR "unit tests and fuzzing tool are exclusive options.")
endif()

if(WASMEDGE_BUILD_STATIC_LIB)
  # Static library will forcefully turn of the LTO.
  set(WASMEDGE_FORCE_DISABLE_LTO ON)
endif()
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
if(WASMEDGE_BUILD_PLUGINS)
  if(WASMEDGE_BUILD_STATIC_LIB)
    set(WASMEDGE_LINK_PLUGINS_STATIC ON)
  endif()
endif()

set(WASMEDGE_BUILD_PACKAGE "DEB;RPM" CACHE STRING "Package generate types")
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpack_config.cmake)

if(WASMEDGE_BUILD_COVERAGE)
  set(GCOVR_ADDITIONAL_ARGS "--exclude-unreachable-branches;--exclude-throw-branches")
  include(CodeCoverage)
  append_coverage_compiler_flags()
endif()

include(Helper)
include(GNUInstallDirs)

string(TOUPPER "${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}" CMAKE_INSTALL_DEFAULT_COMPONENT_NAME_UPCASE)
set(CPACK_PACKAGE_VENDOR Second State LLC)
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_STRIP_FILES ON)
set(CPACK_PACKAGE_CONTACT "Shen-Ta Hsieh <beststeve@secondstate.io>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High performance WebAssembly Virtual Machine")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_ARCHIVE_${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME_UPCASE}_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_RPM_MAIN_COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME})
set(CPACK_RPM_PACKAGE_LICENSE "Apache 2.0")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME_UPCASE}_PACKAGE_NAME wasmedge)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_COMPRESSION_TYPE xz)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/WasmEdge/WasmEdge/")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
set(CPACK_GENERATOR "${WASMEDGE_BUILD_PACKAGE}")
set(CPACK_PACKAGE_DESCRIPTION "WasmEdge is a high performance, extensible, and hardware optimized WebAssembly Virtual Machine for cloud, AI, and blockchain applications.")

if(WASMEDGE_BUILD_TESTS)
  include(CTest)
  add_subdirectory(test)
endif()

add_subdirectory(include)
add_subdirectory(lib)
if(WASMEDGE_BUILD_PLUGINS AND WASMEDGE_BUILD_SHARED_LIB)
  add_subdirectory(plugins)
endif()
add_subdirectory(thirdparty)
if(WASMEDGE_BUILD_TOOLS OR WASMEDGE_BUILD_FUZZING)
  add_subdirectory(tools)
endif()
if(WASMEDGE_BUILD_EXAMPLE)
  add_subdirectory(examples/plugin/get-string)
endif()
add_subdirectory(rpm)

include(CPack)
include(CPackComponent)

