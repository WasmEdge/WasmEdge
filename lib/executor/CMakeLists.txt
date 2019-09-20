add_subdirectory(entry)
add_subdirectory(instance)
add_subdirectory(instantiate)

add_library(ssvmExecutorWorker
  worker.cpp
  worker/control.cpp
  worker/variable.cpp
  worker/memory.cpp
  worker/relation_numeric.cpp
  worker/unary_numeric.cpp
  worker/binary_numeric.cpp
  worker/cast_numeric.cpp
  worker/provider.cpp
)

add_library(ssvmExecutor
  stackmgr.cpp
  storemgr.cpp
  executor.cpp
)
