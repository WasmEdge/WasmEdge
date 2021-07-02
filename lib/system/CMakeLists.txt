# SPDX-License-Identifier: Apache-2.0

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(mmap sys/mman.h HAVE_MMAP)
include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(pwd.h HAVE_PWD_H)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_library(wasmedgeSystem
  allocator.cpp
  fault.cpp
  mmap.cpp
  path.cpp
)

target_compile_options(wasmedgeSystem
  PRIVATE
  ${WASMEDGE_CFLAGS}
)

target_include_directories(wasmedgeSystem
  PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(wasmedgeSystem
  PUBLIC
  wasmedgeCommon
  Boost::boost
)
