add_library(ssvmExecutor
  executor.cpp
  entry/frame.cpp
  entry/label.cpp
  entry/value.cpp
<<<<<<< HEAD
  instance/function.cpp
  instance/global.cpp
  instance/memory.cpp
  instance/module.cpp
  instance/table.cpp
  stackmgr.cpp
  storemgr.cpp
=======
  functioninst.cpp
  globalinst.cpp
  memoryinst.cpp
  moduleinst.cpp
  stackmgr.cpp
  storemgr.cpp
  tableinst.cpp
>>>>>>> [Executor] Move entries into new path and new namespace.
)
