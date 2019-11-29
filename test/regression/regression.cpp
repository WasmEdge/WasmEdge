// SPDX-License-Identifier: Apache-2.0
#include "regression.h"

bool RegressionTester::setExpectResult(SSVM::Result &Res) {
  ExpectResult = Res;
  return true;
}

bool RegressionTester::setFilePath(const std::string &FilePath) {
  WasmPath = FilePath;
  return true;
}

bool RegressionTester::runLoader() {
  LoaderEngine.setPath(WasmPath);
  LoaderEngine.parseModule();
  LoaderEngine.validateModule();
  LoaderEngine.getModule(OutModule);
  return true;
}

bool RegressionTester::runExecutor() {
  /// TODO: prepare arguments
  // Executor.setArguments();
  Executor.instantiate();
  Executor.run();
  return true;
}

bool RegressionTester::checkResult() { return true; }
