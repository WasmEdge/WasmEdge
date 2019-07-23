add_library(ssvmLoaderAST
  module.cpp
  section.cpp
  description.cpp
  segment.cpp
  type.cpp
  expression.cpp
  instruction.cpp
)

add_library(ssvmLoaderFileMgr
  filemgr.cpp
)

add_library(ssvmLoader
  loader.cpp
)