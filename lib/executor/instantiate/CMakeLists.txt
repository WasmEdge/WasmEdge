add_library(ssvmExecutorInstantiate
  import.cpp
  type.cpp
  function.cpp
  global.cpp
  table.cpp
  memory.cpp
  export.cpp
  module.cpp
)

target_link_libraries(ssvmExecutorInstantiate
  PRIVATE
  ssvmExecutorEntry
  ssvmExecutorInstance
  ssvmExecutorMemoryPool
)