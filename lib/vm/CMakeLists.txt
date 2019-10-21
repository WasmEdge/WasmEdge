add_subdirectory(hostfunc)

add_library(ssvmVM
  vm.cpp
)

target_link_libraries(ssvmVM
  PRIVATE
  ssvmASTLoad
  ssvmLoader
  ssvmExecutor
  ssvmHostFuncEEI
  ssvmHostFuncWasi
)
