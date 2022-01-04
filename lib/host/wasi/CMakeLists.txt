# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

if(APPLE)
  set(MACOS_MM macos.mm)
else()
  set(MACOS_MM)
endif()

wasmedge_add_library(wasmedgeHostModuleWasi
  clock-linux.cpp
  clock-macos.cpp
  clock-win.cpp
  environ-linux.cpp
  environ-macos.cpp
  environ-win.cpp
  environ.cpp
  inode-linux.cpp
  inode-macos.cpp
  inode-win.cpp
  vfs.cpp
  vinode.cpp
  wasifunc.cpp
  wasimodule.cpp
  win.cpp
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

if(NOT APPLE AND NOT WIN32)
  target_link_libraries(wasmedgeHostModuleWasi
    PUBLIC
    rt
  )
endif()
