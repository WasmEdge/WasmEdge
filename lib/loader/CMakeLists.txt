add_library(ssvmLoaderFileMgr
  filemgr.cpp
)

add_library(ssvmLoader
  loader.cpp
)

target_link_libraries(ssvmLoader
  PRIVATE
  ssvmLoaderFileMgr
)