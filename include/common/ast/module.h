// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/ast/module.h - Module class definition ----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Module node class, which is the
/// module node in AST.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "base.h"
#include "loader/ldmgr.h"
#include "section.h"

#include <memory>
#include <vector>

namespace SSVM::Interpreter {
class Interpreter;
} // namespace SSVM::Interpreter

namespace SSVM {
namespace AST {

/// AST Module node.
class Module : public Base {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read the Magic and Version sequences.
  /// Read Section indices and create Section nodes.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns void when success, ErrMsg when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Load compiled function from loadable manager.
  Expect<void> loadCompiled(LDMgr &Mgr);

  /// Getter of pointer to sections.
  CustomSection *getCustomSection() const { return CustomSec.get(); }
  TypeSection *getTypeSection() const { return TypeSec.get(); }
  ImportSection *getImportSection() const { return ImportSec.get(); }
  FunctionSection *getFunctionSection() const { return FunctionSec.get(); }
  TableSection *getTableSection() const { return TableSec.get(); }
  MemorySection *getMemorySection() const { return MemorySec.get(); }
  GlobalSection *getGlobalSection() const { return GlobalSec.get(); }
  ExportSection *getExportSection() const { return ExportSec.get(); }
  StartSection *getStartSection() const { return StartSec.get(); }
  ElementSection *getElementSection() const { return ElementSec.get(); }
  CodeSection *getCodeSection() const { return CodeSec.get(); }
  DataSection *getDataSection() const { return DataSec.get(); }

  using TrapProxy = void (*)(Interpreter::Interpreter *, uint32_t);
  using CallProxy = void (*)(Interpreter::Interpreter *, const uint32_t,
                             const ValVariant *, ValVariant *);
  using MemGrowProxy = uint32_t (*)(Interpreter::Interpreter *, const uint32_t);
  using MemSizeProxy = uint32_t (*)(Interpreter::Interpreter *);
  using Ctor = void (*)(TrapProxy, CallProxy, MemGrowProxy, MemSizeProxy);

  Ctor getCtor() const { return CtorFunc; }
  void setCtor(Ctor F) { CtorFunc = F; }

protected:
  /// The node type should be Attr::Module.
  Attr NodeAttr = Attr::Module;

private:
  /// \name Data of Module node.
  /// @{
  std::vector<Byte> Magic;
  std::vector<Byte> Version;
  /// @}

  /// \name Section nodes of Module node.
  /// @{
  std::unique_ptr<CustomSection> CustomSec;
  std::unique_ptr<TypeSection> TypeSec;
  std::unique_ptr<ImportSection> ImportSec;
  std::unique_ptr<FunctionSection> FunctionSec;
  std::unique_ptr<TableSection> TableSec;
  std::unique_ptr<MemorySection> MemorySec;
  std::unique_ptr<GlobalSection> GlobalSec;
  std::unique_ptr<ExportSection> ExportSec;
  std::unique_ptr<StartSection> StartSec;
  std::unique_ptr<ElementSection> ElementSec;
  std::unique_ptr<CodeSection> CodeSec;
  std::unique_ptr<DataSection> DataSec;
  /// @}

  Ctor CtorFunc = nullptr;
};

} // namespace AST
} // namespace SSVM
