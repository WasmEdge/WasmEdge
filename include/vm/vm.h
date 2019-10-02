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
#include "executor/entry/value.h"
#include "executor/executor.h"
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
  VM() = default;
  ~VM() = default;

  /// Set the wasm file path.
  ErrCode setPath(const std::string &FilePath);

  /// Set the call data vector.
  ErrCode setCallData(std::vector<unsigned char> &Data) {
    CallData = Data;
    return ErrCode::Success;
  }

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

  std::string WasmPath;
  Loader::Loader LoaderEngine;
  Executor::Executor ExecutorEngine;
  std::unique_ptr<AST::Module> Mod = nullptr;
  std::vector<std::unique_ptr<Executor::ValueEntry>> Args;
  std::vector<unsigned char> CallData;
  Result VMResult;
};

} // namespace VM
} // namespace SSVM
