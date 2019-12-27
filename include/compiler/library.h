// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/compiler/library.h - Library class definition ----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of Library class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/common.h"
#include "common.h"
#include "hostfunc.h"
#include <memory>
#include <string>
#include <vector>

namespace llvm {
class LLVMContext;
class Module;
class StringRef;
} // namespace llvm

namespace SSVM {
namespace Compiler {

/// Library class
class Library {
private:
  friend class Compiler;
  Library();
  void setModule(std::unique_ptr<llvm::Module> Module);
  llvm::LLVMContext &getContext();

public:
  Library(const Library &) = delete;
  ~Library() noexcept;

  /// Set host function.
  ErrCode setHostFunction(std::unique_ptr<HostFunction> Func,
                          const std::string &ModName,
                          const std::string &FuncName);
  template <typename T, typename... ArgsT>
  std::enable_if_t<std::is_base_of_v<HostFunction, T>, ErrCode>
  setHostFunction(const std::string &ModName, const std::string &FuncName,
                  ArgsT &&... Args) {
    return setHostFunction(
        std::make_unique<T>(*this, std::forward<ArgsT>(Args)...), ModName,
        FuncName);
  }

  /// Append the start function arguments.
  ErrCode appendArgument(AST::ValVariant Val) {
    Arguments.push_back(std::move(Val));
    return ErrCode::Success;
  }

  /// Get start function return values.
  const std::vector<AST::ValVariant> &getReturnValue() const { return Returns; }

  /// Execute wasm with given input.
  ErrCode execute();
  ErrCode execute(const std::string &FuncName);

  void terminate();

  template <typename T> T &getMemory(uint32_t Offset) {
    return *reinterpret_cast<T *>(Memory.data() + Offset);
  }

  template <typename T>
  Span<T *> getMemory(uint32_t Offset, uint32_t Length) {
    const auto Begin = reinterpret_cast<T *>(Memory.data() + Offset);
    const auto End = Begin + Length;
    return {Begin, End};
  }

private:
  class Engine;
  Engine *ExecutionEngine;
  std::vector<AST::ValVariant> Arguments;
  std::vector<AST::ValVariant> Returns;
  std::vector<std::unique_ptr<HostFunction>> HostFuncs;
  std::vector<uint8_t> Memory;
  void *MemoryPtr;

  void trap(ErrCode Status);
  uint32_t memorySize();
  uint32_t memoryGrow(uint32_t NewSize);
  static void trapProxy(Library *Lib, ErrCode Status) {
    Lib->trap(Status);
  }
  static uint32_t memorySizeProxy(Library *Lib) {
    return Lib->memorySize();
  }
  static uint32_t memoryGrowProxy(Library *Lib, uint32_t NewSize) {
    return Lib->memoryGrow(NewSize);
  }
};

} // namespace Compiler
} // namespace SSVM
