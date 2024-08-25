# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

if(APPLE)
  set(WASMEDGE_WASI_SRCS clock-macos.cpp environ-macos.cpp inode-macos.cpp macos.mm)
elseif(WIN32)
  set(WASMEDGE_WASI_SRCS clock-win.cpp environ-win.cpp inode-win.cpp)
else()
  set(WASMEDGE_WASI_SRCS clock-linux.cpp environ-linux.cpp inode-linux.cpp)
endif()

wasmedge_add_library(wasmedgeHostModuleWasi
  environ.cpp
  vinode.cpp
  wasifunc.cpp
  wasimodule.cpp
  ${WASMEDGE_WASI_SRCS}
)

target_include_directories(wasmedgeHostModuleWasi
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(wasmedgeHostModuleWasi
  PUBLIC
  Threads::Threads
  wasmedgeSystem
)

if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
  target_link_libraries(wasmedgeHostModuleWasi
    PUBLIC
    rt
  )
endif()

if(WIN32)
  target_link_libraries(wasmedgeHostModuleWasi
  PRIVATE
  ktmw32
  ntdll
  wsock32
  ws2_32
  pathcch
  shlwapi
)
endif()
