//===-- ssvm/executor/executor.h - Executor flow control class definition -===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Executor class, which controls
/// flow of WASM execution.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common.h"
#include "hostfunc.h"
#include "hostfuncmgr.h"
#include "stackmgr.h"
#include "storemgr.h"
#include "worker.h"
#include <memory>

namespace SSVM {
namespace Executor {

/// Executor flow control class.
class Executor {
public:
  Executor() : Engine(StoreMgr, StackMgr, HostFuncMgr) {}
  ~Executor() = default;

  /// Set host functions.
  ErrCode setHostFunction(std::unique_ptr<HostFunction> &Func,
                          const std::string &ModName,
                          const std::string &FuncName);

  /// Retrieve ownership of Wasm Module.
  ErrCode setModule(std::unique_ptr<AST::Module> &Module);

  /// Instantiate Wasm Module.
  ErrCode instantiate();

  /// Set start function arguments.
  ErrCode setArgs(std::vector<std::unique_ptr<ValueEntry>> &Args);

  /// Execute Wasm.
  ErrCode run();

  /// Reset Executor.
  ErrCode reset(bool Force = false);

private:
  /// Instantiation of Module Instance.
  ErrCode instantiate(AST::Module *Mod);

  /// Instantiation of Import Section.
  ErrCode instantiate(AST::ImportSection *ImportSec);

  /// Instantiation of types in Module Instance.
  ErrCode instantiate(AST::TypeSection *TypeSec);

  /// Instantiation of Function Instances.
  ErrCode instantiate(AST::FunctionSection *FuncSec, AST::CodeSection *CodeSec);

  /// Instantiation of Global Instances.
  ErrCode instantiate(AST::GlobalSection *GlobSec);

  /// Instantiation of Table Instances.
  ErrCode instantiate(AST::TableSection *TabSec, AST::ElementSection *ElemSec);

  /// Instantiation of Memory Instances.
  ErrCode instantiate(AST::MemorySection *MemSec, AST::DataSection *DataSec);

  /// Instantiation of Export Instances.
  ErrCode instantiate(AST::ExportSection *ExportSec);

  /// Executor State
  enum class State : unsigned int {
    Inited,
    ModuleSet,
    Instantiated,
    ArgsSet,
    Finished
  };

  State Stat = State::Inited;
  std::unique_ptr<AST::Module> Mod = nullptr;
  Instance::ModuleInstance *ModInst = nullptr;
  Worker Engine;
  StackManager StackMgr;
  StoreManager StoreMgr;
  HostFunctionManager HostFuncMgr;
};

} // namespace Executor
} // namespace SSVM
