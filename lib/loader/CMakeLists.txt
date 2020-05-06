# SPDX-License-Identifier: Apache-2.0

add_library(ssvmLoaderFileMgr
  filemgr.cpp
)

target_include_directories(ssvmLoaderFileMgr
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)

add_library(ssvmLoader
  loader.cpp
)

target_link_libraries(ssvmLoader
  PRIVATE
  ssvmAST
  ssvmLoaderFileMgr
)
