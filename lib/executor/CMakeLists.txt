add_subdirectory(entry)
add_subdirectory(instance)
add_subdirectory(instantiate)

add_library(ssvmExecutorWorker
  worker.cpp
  worker/control.cpp
  worker/variable.cpp
  worker/memory.cpp
  worker/provider.cpp
)

add_library(ssvmExecutor
  executor.cpp
)
