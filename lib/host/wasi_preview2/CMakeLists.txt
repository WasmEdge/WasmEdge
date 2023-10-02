# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2023 Second State INC

wasmedge_add_library(wasmedgeHostModuleWasiPreview2
  environ.cpp
  wasifunc.cpp
  wasimodule.cpp
  ${WASMEDGE_WASI_SRCS}
)

target_include_directories(wasmedgeHostModuleWasiPreview2
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(wasmedgeHostModuleWasiPreview2
  PUBLIC
  Threads::Threads
  wasmedgeSystem
)

if(NOT APPLE AND NOT WIN32 AND NOT ANDROID)
  target_link_libraries(wasmedgeHostModuleWasiPreview2
    PUBLIC
    rt
  )
endif()

if(WIN32)
  target_link_libraries(wasmedgeHostModuleWasiPreview2
  PRIVATE
  ntdll
  wsock32
  ws2_32
  pathcch
  shlwapi
)
endif()
