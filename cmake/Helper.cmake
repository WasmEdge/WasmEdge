# SPDX-License-Identifier: Apache-2.0

set(WASMEDGE_INTERPROCEDURAL_OPTIMIZATION OFF)
if(CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
  if(NOT WASMEDGE_FORCE_DISABLE_LTO)
    set(WASMEDGE_INTERPROCEDURAL_OPTIMIZATION ON)
  endif()
  if (CMAKE_GENERATOR STREQUAL Ninja)
    if(CMAKE_COMPILER_IS_GNUCXX)
      list(TRANSFORM CMAKE_C_COMPILE_OPTIONS_IPO REPLACE "^-flto$" "-flto=auto")
      list(TRANSFORM CMAKE_CXX_COMPILE_OPTIONS_IPO REPLACE "^-flto$" "-flto=auto")
    endif()
    set(CMAKE_JOB_POOLS "link=2")
    set(CMAKE_JOB_POOL_LINK link)
  endif()
endif()

list(APPEND WASMEDGE_CFLAGS
  -Wall
  -Wextra
  -Wno-error=pedantic
)
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  list(APPEND WASMEDGE_CFLAGS -Wno-psabi)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND WASMEDGE_CFLAGS -Wno-c++20-designator)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  list(APPEND WASMEDGE_CFLAGS
    -Wno-c99-extensions
    -Wno-covered-switch-default
    -Wno-documentation-unknown-command
    -Wno-keyword-macro
    -Wno-newline-eof
    -Wno-return-std-move-in-c++11
    -Wno-shadow-field-in-constructor
    -Wno-signed-enum-bitfield
    -Wno-switch-enum
    -Wno-undefined-func-template
  )
endif()

if(WIN32)
  add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE -DNOMINMAX -D_ITERATOR_DEBUG_LEVEL=0)
  list(APPEND WASMEDGE_CFLAGS
    "/EHa"
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-exit-time-destructors
    -Wno-global-constructors
    -Wno-used-but-marked-unused
    -Wno-nonportable-system-include-path
    -Wno-float-equal
  )
endif()

function(wasmedge_setup_target target)
  set_target_properties(${target} PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    ENABLE_EXPORTS ON
    POSITION_INDEPENDENT_CODE ON
    VISIBILITY_INLINES_HIDDEN ON
    SKIP_RPATH ON
    INTERPROCEDURAL_OPTIMIZATION ${WASMEDGE_INTERPROCEDURAL_OPTIMIZATION}
  )
  target_compile_options(${target}
    PRIVATE
    ${WASMEDGE_CFLAGS}
  )
endfunction()

function(wasmedge_add_library target)
  add_library(${target} ${ARGN})
  wasmedge_setup_target(${target})
endfunction()

function(wasmedge_add_executable target)
  add_executable(${target} ${ARGN})
  wasmedge_setup_target(${target})
endfunction()
