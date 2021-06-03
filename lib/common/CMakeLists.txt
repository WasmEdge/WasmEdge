# SPDX-License-Identifier: Apache-2.0

find_package(spdlog QUIET)
if(spdlog_FOUND)
else()
  FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.8.5
    GIT_SHALLOW    TRUE
  )
  set(BUILD_GMOCK OFF)
  FetchContent_MakeAvailable(spdlog)
endif()

add_library(wasmedgeCommon
  hexstr.cpp
  log.cpp
  configure.cpp
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
