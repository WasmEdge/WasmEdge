// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/vm/vm.h - VM execution flow class definition -----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of VM class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common.h"
#include "configure.h"
#include "environment.h"
#include "executor/entry/value.h"
#include "executor/executor.h"
#include "executor/hostfunc.h"
#include "loader/loader.h"
#include "result.h"
#include "support/casting.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace SSVM {
namespace VM {

namespace {
/// Accept host functions.
template <typename T, typename TR>
using TypeFunc =
    typename std::enable_if_t<std::is_base_of_v<Executor::HostFunction, T>, TR>;
/// Return environments.
template <typename T>
using TypeEnv = typename std::enable_if_t<std::is_base_of_v<Environment, T>, T>;
} // namespace

/// VM execution flow class
class VM {
public:
  VM() = delete;
  VM(Configure &InputConfig);
  ~VM() = default;

  /// Set the wasm file path.
  ErrCode setPath(const std::string &FilePath);

  /// Set the wasm byte code.
  ErrCode setCode(const std::vector<uint8_t> &Code);

  /// Set host function.
  template <typename T>
  TypeFunc<T, ErrCode> setHostFunction(std::unique_ptr<T> &Func,
                                       const std::string &ModName,
                                       const std::string &FuncName) {
    std::unique_ptr<Executor::HostFunction> NewFunc = std::move(Func);
    if (ExecutorEngine.setHostFunction(NewFunc, ModName, FuncName) ==
        Executor::ErrCode::Success) {
      return ErrCode::Success;
    }
    return ErrCode::Failed;
  }

  /// Append the start function arguments.
  template <typename T>
  typename std::enable_if_t<Support::IsWasmBuiltInV<T>, ErrCode>
  appendArgument(const T &Val) {
    Args.push_back(Val);
    return ErrCode::Success;
  }

  /// Execute wasm with given input.
  ErrCode execute();
  ErrCode execute(const std::string &FuncName);

  /// Return VMResult
  Result getResult() { return VMResult; }

  /// Getter of Environment.
  template <typename T> TypeEnv<T> *getEnvironment(Configure::VMType Type) {
    if (EnvTable.find(Type) != EnvTable.end()) {
      return dynamic_cast<T *>(EnvTable[Type].get());
    }
    return nullptr;
  }

private:
  /// Functions for running.
  ErrCode runLoader();
  ErrCode runExecutor();

  /// Helper function for inserting host functions according to VM type.
  ErrCode prepareVMHost();

  /// Wasm source.
  std::string WasmPath;
  std::vector<uint8_t> WasmCode;

  Loader::Loader LoaderEngine;
  Executor::Executor ExecutorEngine;
  Configure &Config;
  std::unordered_map<Configure::VMType, std::unique_ptr<Environment>> EnvTable;
  std::unique_ptr<AST::Module> Mod;
  std::vector<Executor::Value> Args;
  std::vector<Executor::Value> Rets;
  Result VMResult;
};

} // namespace VM
} // namespace SSVM
