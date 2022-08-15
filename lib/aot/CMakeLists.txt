# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

find_package(LLVM REQUIRED HINTS "${LLVM_CMAKE_PATH}")
list(APPEND CMAKE_MODULE_PATH ${LLVM_DIR})
include(LLVMConfig)
include(AddLLVM)

find_package(LLD REQUIRED HINTS "${LLVM_LIBRARY_DIR}/cmake/lld")

if(WASMEDGE_LINK_LLVM_STATIC)
  wasmedge_add_library(wasmedgeAOT
    blake3.cpp
    cache.cpp
    compiler.cpp
  )

  target_link_libraries(wasmedgeAOT
    PUBLIC
    wasmedgeCommon
    wasmedgeSystem
    utilBlake3
    std::filesystem
    ${WASMEDGE_LLVM_LINK_STATIC_COMPONENTS}
    ${WASMEDGE_LLVM_LINK_SHARED_COMPONENTS}
  )
else()
  if(APPLE)
    list(APPEND LLD_LIBS lldMachO)
  elseif(WIN32)
    list(APPEND LLD_LIBS lldCOFF)
    set(EXTRA_COMPONENTS DebugInfoPDB LibDriver WindowsManifest)
  else()
    list(APPEND LLD_LIBS lldELF)
  endif()

  list(APPEND LLD_LIBS lldCommon)
  if(LLVM_VERSION_MAJOR LESS_EQUAL 13)
    list(APPEND LLD_LIBS lldDriver)
  endif()
  
  llvm_add_library(wasmedgeAOT
    blake3.cpp
    cache.cpp
    compiler.cpp
    LINK_LIBS
    wasmedgeCommon
    wasmedgeSystem
    utilBlake3
    ${LLD_LIBS}
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
endif()

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
