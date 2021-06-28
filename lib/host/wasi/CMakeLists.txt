# SPDX-License-Identifier: Apache-2.0

add_library(wasmedgeHostModuleWasi
  wasienv.cpp
  wasifunc.cpp
  wasimodule.cpp
)

target_include_directories(wasmedgeHostModuleWasi
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(wasmedgeHostModuleWasi
  PUBLIC
  Threads::Threads
  Boost::boost
  wasmedgeSystem
)

if(NOT CMAKE_SYSTEM_NAME STREQUAL Darwin)
  target_link_libraries(wasmedgeHostModuleWasi
    PUBLIC
    rt
  )
endif()
