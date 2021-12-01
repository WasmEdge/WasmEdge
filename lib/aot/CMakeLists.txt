# SPDX-License-Identifier: Apache-2.0

find_package(LLVM REQUIRED HINTS "${LLVM_CMAKE_PATH}")
list(APPEND CMAKE_MODULE_PATH ${LLVM_DIR})
include(LLVMConfig)
include(AddLLVM)

find_library(LLD_COMMON lldCommon PATHS "${LLVM_LIBRARY_DIR}")
find_library(LLD_CORE lldCore PATHS "${LLVM_LIBRARY_DIR}")
find_library(LLD_DRIVER lldDriver PATHS "${LLVM_LIBRARY_DIR}")
find_library(LLD_READERWRITER lldReaderWriter PATHS "${LLVM_LIBRARY_DIR}")
find_library(LLD_YAML lldYAML PATHS "${LLVM_LIBRARY_DIR}")
if(APPLE)
  find_library(LLD_SYSTEM lldMachO PATHS "${LLVM_LIBRARY_DIR}")
elseif(WIN32)
  find_library(LLD_SYSTEM lldCOFF PATHS "${LLVM_LIBRARY_DIR}")
  set(EXTRA_COMPONENTS DebugInfoPDB LibDriver WindowsManifest)
else()
  find_library(LLD_SYSTEM lldELF PATHS "${LLVM_LIBRARY_DIR}")
endif()

llvm_add_library(wasmedgeAOT
  blake3.cpp
  cache.cpp
  compiler.cpp
  LINK_LIBS
  wasmedgeCommon
  wasmedgeSystem
  utilBlake3
  ${LLD_SYSTEM}
  ${LLD_COMMON}
  ${LLD_CORE}
  ${LLD_DRIVER}
  ${LLD_READERWRITER}
  ${LLD_YAML}
  std::filesystem
  ${CMAKE_THREAD_LIBS_INIT}
  LINK_COMPONENTS
  core
  lto
  native
  nativecodegen
  option
  passes
  support
  transformutils
  ${EXTRA_COMPONENTS}
)

wasmedge_setup_target(wasmedgeAOT)

target_include_directories(wasmedgeAOT
  SYSTEM
  PRIVATE
  ${LLVM_INCLUDE_DIR}
)

target_include_directories(wasmedgeAOT
  PUBLIC
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty/blake3
)
