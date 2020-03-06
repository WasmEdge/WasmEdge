# SPDX-License-Identifier: Apache-2.0

add_library(ssvmLoaderFileMgr
  filemgr.cpp
)

add_library(ssvmLoader
  loader.cpp
)

target_link_libraries(ssvmLoader
  PRIVATE
  ssvmAST
  ssvmLoaderFileMgr
)
