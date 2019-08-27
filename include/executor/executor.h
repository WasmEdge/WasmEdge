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
#include "stackmgr.h"
#include "storemgr.h"
#include "worker.h"
#include <memory>

namespace SSVM {
namespace Executor {

/// Executor flow control class.
class Executor {
public:
  Executor() = default;
  ~Executor() = default;

  /// Retrieve ownership of Wasm Module.
  ErrCode setModule(std::unique_ptr<AST::Module> &Module);

  /// Instantiate Wasm Module.
  ErrCode instantiate() { return instantiate(Mod.get()); }

  /// Execute Wasm.
  ErrCode run();

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

  std::unique_ptr<AST::Module> Mod = nullptr;
  unsigned int ModInstId = 0;
  StackManager StackMgr;
  StoreManager StoreMgr;
};

} // namespace Executor
} // namespace SSVM
