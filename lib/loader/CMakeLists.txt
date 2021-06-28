# SPDX-License-Identifier: Apache-2.0

add_library(wasmedgeLoaderFileMgr
  filemgr.cpp
  ldmgr.cpp
  shared_library.cpp
)

target_link_libraries(wasmedgeLoaderFileMgr
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
  dl
  std::filesystem
)

add_library(wasmedgeLoader
  loader.cpp
)

target_link_libraries(wasmedgeLoader
  PUBLIC
  wasmedgeCommon
  wasmedgeAST
  wasmedgeLoaderFileMgr
  Boost::boost
  std::filesystem
)
