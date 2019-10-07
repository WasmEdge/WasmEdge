//===-- ssvm/vm/vm.h - VM execution flow class definition -----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
///
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common.h"
#include "environment.h"
#include "executor/entry/value.h"
#include "executor/executor.h"
#include "executor/hostfunc.h"
#include "loader/loader.h"
#include "result.h"
#include "support/casting.h"

#include <cstdint>
#include <string>
#include <vector>

namespace SSVM {
namespace VM {

/// VM execution flow class
class VM {
public:
  VM() = delete;
  VM(Environment &InputEnv);
  ~VM() = default;

  /// Set the wasm file path.
  ErrCode setPath(const std::string &FilePath);

  /// Set the call data vector.
  ErrCode setCallData(std::vector<unsigned char> &Data);

  /// Append the start function arguments.
  template <typename T>
  typename std::enable_if_t<Support::IsWasmBuiltInV<T>, ErrCode>
  appendArgument(const T &Val) {
    Args.push_back(std::make_unique<Executor::ValueEntry>(Val));
    return ErrCode::Success;
  }

  /// Execute wasm with given input.
  ErrCode execute();

  /// Return VMResult
  Result getResult() { return VMResult; }

private:
  /// Functions for running.
  ErrCode runLoader();
  ErrCode runExecutor();

  template <typename T>
  std::unique_ptr<Executor::HostFunction>
  castHostFunc(std::unique_ptr<T> Func) {
    T *FuncPtr = Func.release();
    return std::move(std::unique_ptr<Executor::HostFunction>(
        dynamic_cast<Executor::HostFunction *>(FuncPtr)));
  }

  std::string WasmPath;
  Loader::Loader LoaderEngine;
  Executor::Executor ExecutorEngine;
  Environment Env;
  std::unique_ptr<AST::Module> Mod = nullptr;
  std::vector<std::unique_ptr<Executor::ValueEntry>> Args;
  Result VMResult;
};

} // namespace VM
} // namespace SSVM
