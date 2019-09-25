add_executable(ssvmRegressionTests
  regression.cpp
  main.cpp
)

target_link_libraries(ssvmRegressionTests
  PRIVATE
  utilGoogleTest
  ssvmASTLoad
  ssvmLoaderFileMgr
  ssvmLoader
  ssvmExecutorWorker
  ssvmExecutorInstance
  ssvmExecutorEntry
  ssvmExecutorInstantiate
  ssvmExecutor
)