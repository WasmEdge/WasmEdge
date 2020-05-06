# SPDX-License-Identifier: Apache-2.0

add_library(ssvmHostFuncWasi
  args_Get.cpp
  args_SizesGet.cpp
  environ_Get.cpp
  environ_SizesGet.cpp
  fd_Close.cpp
  fd_FdstatGet.cpp
  fd_FdstatSetFlags.cpp
  fd_PrestatDirName.cpp
  fd_PrestatGet.cpp
  fd_Read.cpp
  fd_Seek.cpp
  fd_Write.cpp
  path_Open.cpp
  proc_Exit.cpp
)

target_include_directories(ssvmHostFuncWasi
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
  ${PROJECT_SOURCE_DIR}/thirdparty/evmc/include
)
