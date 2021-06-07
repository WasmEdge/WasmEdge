# SPDX-License-Identifier: Apache-2.0

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(mmap sys/mman.h HAVE_MMAP)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_library(wasmedgeSystem
  allocator.cpp
  mmap.cpp
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
