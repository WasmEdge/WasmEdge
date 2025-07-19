# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgeHostModuleWasiSockets
  wasifunc.cpp
  inode-linux.cpp
  udp.cpp
  tcp.cpp
  ipnamelookup.cpp
  util.cpp
  ${WASMEDGE_WASI_SRCS}
)

target_include_directories(wasmedgeHostModuleWasiSockets
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(wasmedgeHostModuleWasiSockets
  PUBLIC
  Threads::Threads
  wasmedgeSystem
)

#if(WIN32)
#  target_link_libraries(wasmedgeHostModuleWasiSockets
#  PRIVATE
#  ntdll
#  wsock32
#  ws2_32
#  pathcch
#  shlwapi
#)
#endif()
