# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

find_package(spdlog QUIET)
if(spdlog_FOUND)
else()
  FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.9.1
    GIT_SHALLOW    TRUE
  )
  set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    get_property(
      compile_options
      DIRECTORY
      PROPERTY COMPILE_OPTIONS
    )
    set_property(
      DIRECTORY
      APPEND
      PROPERTY COMPILE_OPTIONS
      ${WASMEDGE_CFLAGS}
      -Wno-missing-noreturn
      -Wno-missing-variable-declarations
    )
    unset(compile_options)
  endif()
  FetchContent_MakeAvailable(spdlog)
  wasmedge_setup_target(spdlog)
endif()

wasmedge_add_library(wasmedgeCommon
  hexstr.cpp
  log.cpp
  errinfo.cpp
  int128.cpp
)

target_link_libraries(wasmedgeCommon
  PUBLIC
  spdlog::spdlog
)

target_include_directories(wasmedgeCommon
  PUBLIC
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_SOURCE_DIR}/include
)
