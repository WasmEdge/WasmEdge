# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

find_package(spdlog QUIET)
if(spdlog_FOUND)
else()
  FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        10.2.1
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(fmt)
  wasmedge_setup_target(fmt)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(fmt
      PUBLIC
      -Wno-missing-noreturn
      PRIVATE
      -Wno-sign-conversion
    )
  endif()

  FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.13.0
    GIT_SHALLOW    TRUE
  )
  set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE)
  set(SPDLOG_FMT_EXTERNAL ON  CACHE BOOL "Use external fmt library instead of bundled" FORCE)
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
