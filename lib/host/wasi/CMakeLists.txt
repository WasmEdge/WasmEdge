# SPDX-License-Identifier: Apache-2.0

if(APPLE)
  set(MACOS_MM macos.mm)
else()
  set(MACOS_MM)
endif()

add_library(wasmedgeHostModuleWasi
  clock-linux.cpp
  clock-macos.cpp
  environ-linux.cpp
  environ-macos.cpp
  environ.cpp
  inode-linux.cpp
  inode-macos.cpp
  vfs.cpp
  vinode.cpp
  wasifunc.cpp
  wasimodule.cpp
  ${MACOS_MM}
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

if(NOT APPLE)
  target_link_libraries(wasmedgeHostModuleWasi
    PUBLIC
    rt
  )
endif()
