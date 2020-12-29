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
else()
  find_library(LLD_SYSTEM lldELF PATHS "${LLVM_LIBRARY_DIR}")
endif()

llvm_add_library(ssvmAOT
  compiler.cpp
  LINK_LIBS
  ssvmCommon
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
)

target_include_directories(ssvmAOT
  SYSTEM
  PRIVATE
  "${LLVM_INCLUDE_DIR}"
)

target_include_directories(ssvmAOT
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
