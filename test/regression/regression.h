#pragma once

#include "executor/executor.h"
#include "loader/loader.h"
#include "result.h"

class RegressionTester {
public:
  RegressionTester() = default;
  ~RegressionTester() = default;

  /// Functions to set expect results.
  bool setExpectDBMgr(DBMgr &Mgr);
  bool setExpectResult(Result &Res);

  /// Functions to set inputs.
  bool setFilePath(const std::string &FilePath);
  bool setDBMgr(DBMgr &Mgr);

  /// Functions for running.
  bool runLoader();
  bool runExecutor();
  bool checkResult();

private:
  Loader::Loader LoaderEngine;
  Executor::Executor ExecutorEngine;
  std::unique_ptr<AST::Module> OutModule;
  std::string WasmPath;
  DBMgr InitDB;
  DBMgr ExpectDB;
  Result InitResult;
  Result ExpectResult;
};