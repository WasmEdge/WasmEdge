//===-- ssvm/loader/module.h - Module class definition ----------*- C++ -*-===//
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
#include "section.h"

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
  /// \returns true on success.
  virtual Base::ErrCode loadBinary(FileMgr &Mgr);

protected:
  /// The node type should be Attr::Module.
  Attr NodeAttr = Attr::Module;

private:
  /// \name Data of Module node.
  /// @{
  std::vector<unsigned char> Magic;
  std::vector<unsigned char> Version;
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
};

} // namespace AST