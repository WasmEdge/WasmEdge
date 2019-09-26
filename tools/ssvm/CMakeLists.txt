add_executable(ssvm
  main.cpp
)

target_link_libraries(ssvm
  PRIVATE
  ssvmASTLoad
  ssvmLoader
  ssvmExecutor
  ssvmVM
)
