# SPDX-License-Identifier: Apache-2.0

add_library(wasmedgeLoaderFileMgr
  filemgr.cpp
  ldmgr.cpp
  shared_library.cpp
)

target_link_libraries(wasmedgeLoaderFileMgr
  PUBLIC
  dl
  std::filesystem
)

target_link_libraries(wasmedgeLoaderFileMgr
  PRIVATE
  wasmedgeCommon
)

target_include_directories(wasmedgeLoaderFileMgr
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)

add_library(wasmedgeLoader
  loader.cpp
)

target_link_libraries(wasmedgeLoader
  PRIVATE
  wasmedgeAST
  wasmedgeLoaderFileMgr
  wasmedgeCommon
  PUBLIC
  std::filesystem
)

target_include_directories(wasmedgeLoader
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
