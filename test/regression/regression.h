// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/executor.h"
#include "loader/loader.h"
#include "vm/result.h"

class RegressionTester {
public:
  RegressionTester() = default;
  ~RegressionTester() = default;

  /// Functions to set expect results.
  bool setExpectResult(SSVM::Result &Res);

  /// Functions to set inputs.
  bool setFilePath(const std::string &FilePath);

  /// Functions for running.
  bool runLoader();
  bool runExecutor();
  bool checkResult();

private:
  SSVM::Loader::Loader LoaderEngine;
  SSVM::Executor::Executor Executor;
  std::unique_ptr<SSVM::AST::Module> OutModule;
  std::string WasmPath;
  SSVM::Result InitResult;
  SSVM::Result ExpectResult;
};
