# SPDX-License-Identifier: Apache-2.0

add_library(ssvmLoaderFileMgr
  filemgr.cpp
  ldmgr.cpp
)

target_link_libraries(ssvmLoaderFileMgr
  PUBLIC
  dl
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
  ssvmSupport
)

target_include_directories(ssvmLoader
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
