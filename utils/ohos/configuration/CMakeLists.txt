# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

cmake_minimum_required(VERSION 3.10.2)
cmake_policy(SET CMP0091 NEW)

option(WASMEDGE_BUILD_ON_OHOS "Cross compiler WasmEdge on OpenHarmony." OFF)
if(WASMEDGE_BUILD_ON_OHOS)
  set(TOOLSCHAIN_PATH "${OHOS_DIR_PATH}/prebuilts/clang/ohos/linux-x86_64/llvm")
  set(TOOLCHAIN_HOST "${TOOLSCHAIN_PATH}/bin")
  set(OHOS_SYSROOT_PATH "${OHOS_DIR_PATH}/out/ohos-arm-release/obj/third_party/musl")
  set(CMAKE_SYSROOT ${OHOS_SYSROOT_PATH})
  set(CMAKE_CROSSCOMPILING TRUE)
  set(CMAKE_SYSTEM_NAME "Generic")
  set(CMAKE_CXX_COMPILER_ID Clang)
  set(CMAKE_TOOLCHAIN_PREFIX llvm-)
  set(LLVM_PATH "${OHOS_DIR_PATH}/prebuilts/clang/ohos/linux-x86_64/llvm")
  include_directories(${LLVM_PATH}/include/c++/v1)
  include_directories(${OHOS_SYSROOT_PATH}/usr/include/arm-linux-ohosmusl)
  link_directories(${OHOS_SYSROOT_PATH}/usr/lib/arm-linux-ohosmusl)
  set(TOOLCHAIN_CC "${TOOLCHAIN_HOST}/clang")
  set(TOOLCHAIN_CXX "${TOOLCHAIN_HOST}/clang++")
  set(CMAKE_C_COMPILER ${TOOLCHAIN_CC})
  set(CMAKE_C_FLAGS "--target=arm-linux-ohosmusl -D__clang__ -march=armv7-a -mfloat-abi=softfp -mtune=generic-armv7-a -mfpu=neon -mthumb -w --sysroot=${OHOS_SYSROOT_PATH}")
  set(CMAKE_CXX_COMPILER ${TOOLCHAIN_CXX})
  set(CMAKE_CXX_FLAGS "--target=arm-linux-ohosmusl -D__clang__ -march=armv7-a -mfloat-abi=softfp -mtune=generic-armv7-a -mfpu=neon -mthumb -w --sysroot=${OHOS_SYSROOT_PATH}")
  set(MY_LINK_FLAGS "--target=arm-linux-ohosmusl --sysroot=${OHOS_SYSROOT_PATH}")
  set(CMAKE_LINKER clang)
  set(CMAKE_CXX_LINKER clang++)
  set(CMAKE_C_LINKER clang)
  set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINKER} ${MY_LINK_FLAGS} <FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
  set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINKER} ${MY_LINK_FLAGS} <FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
endif()

project(WasmEdge)

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
  set(CPACK_PACKAGE_VERSION LOCAL_VERSION)
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

include(Helper)

# List of WasmEdge options
option(WASMEDGE_BUILD_TESTS "Generate build targets for the wasmedge unit tests." OFF)
option(WASMEDGE_BUILD_COVERAGE "Generate coverage report. Require WASMEDGE_BUILD_TESTS." OFF)
option(WASMEDGE_BUILD_SHARED_LIB "Generate the WasmEdge shared library." ON)
option(WASMEDGE_BUILD_STATIC_LIB "Generate the WasmEdge static library." OFF)
option(WASMEDGE_BUILD_TOOLS "Generate wasmedge and wasmedgec tools." ON)
option(WASMEDGE_USE_LLVM "Enable WasmEdge LLVM-based ahead of time compilation runtime." ON)
option(WASMEDGE_FORCE_DISABLE_LTO "Forcibly disable link time optimization when linking even in Release/RelWithDeb build." OFF)
set(WASMEDGE_BUILD_PACKAGE "DEB;RPM" CACHE STRING "Package generate types")
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpack_config.cmake)

if(WASMEDGE_BUILD_COVERAGE)
  include(CodeCoverage)
  append_coverage_compiler_flags()
endif()

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
add_subdirectory(thirdparty)
if(WASMEDGE_BUILD_TOOLS)
  add_subdirectory(tools)
endif()

include(CPack)
include(CPackComponent)
