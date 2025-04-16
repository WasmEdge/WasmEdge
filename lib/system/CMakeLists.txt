# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgeSystem
  allocator.cpp
  fault.cpp
  mmap.cpp
  path.cpp
  stacktrace.cpp
)

target_include_directories(wasmedgeSystem
  PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(wasmedgeSystem
  PUBLIC
  wasmedgeCommon
)

if(WIN32)
  target_link_libraries(wasmedgeSystem
    PRIVATE
    dbghelp
  )
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  target_link_libraries(wasmedgeSystem
    PRIVATE
    execinfo
  )
endif()
