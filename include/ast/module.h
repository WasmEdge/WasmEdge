// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/ast/module.h - Module class definition -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Module node class, which is the
/// module node in AST.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/base.h"
#include "ast/section.h"
#include "loader/ldmgr.h"

#include <vector>

namespace WasmEdge::Interpreter {
class Interpreter;
} // namespace WasmEdge::Interpreter

namespace WasmEdge {
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
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

  /// Load compiled function from loadable manager.
  Expect<void> loadCompiled(LDMgr &Mgr);

  /// Getters of references of sections.
  const std::vector<CustomSection> &getCustomSections() const {
    return CustomSecs;
  }
  const TypeSection &getTypeSection() const { return TypeSec; }
  const ImportSection &getImportSection() const { return ImportSec; }
  const FunctionSection &getFunctionSection() const { return FunctionSec; }
  const TableSection &getTableSection() const { return TableSec; }
  const MemorySection &getMemorySection() const { return MemorySec; }
  const GlobalSection &getGlobalSection() const { return GlobalSec; }
  const ExportSection &getExportSection() const { return ExportSec; }
  const StartSection &getStartSection() const { return StartSec; }
  const ElementSection &getElementSection() const { return ElementSec; }
  const CodeSection &getCodeSection() const { return CodeSec; }
  const DataSection &getDataSection() const { return DataSec; }
  const DataCountSection &getDataCountSection() const { return DataCountSec; }
  const AOTSection &getAOTSection() const { return AOTSec; }

  enum class Intrinsics : uint32_t {
    kTrap,
    kCall,
    kCallIndirect,
    kMemCopy,
    kMemFill,
    kMemGrow,
    kMemSize,
    kMemInit,
    kDataDrop,
    kTableGet,
    kTableSet,
    kTableCopy,
    kTableFill,
    kTableGrow,
    kTableSize,
    kTableInit,
    kElemDrop,
    kRefFunc,
    kIntrinsicMax,
  };
  using IntrinsicsTable = void * [uint32_t(Intrinsics::kIntrinsicMax)];

  /// Getter of compiled symbol.
  const auto &getSymbol() const noexcept { return IntrSymbol; }
  /// Setter of compiled symbol.
  void setSymbol(Symbol<const IntrinsicsTable *> S) noexcept {
    IntrSymbol = std::move(S);
  }

  /// The node type should be ASTNodeAttr::Module.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Module;

private:
  /// \name Data of Module node.
  /// @{
  std::vector<Byte> Magic;
  std::vector<Byte> Version;
  /// @}

  /// \name Section nodes of Module node.
  /// @{
  std::vector<CustomSection> CustomSecs;
  TypeSection TypeSec;
  ImportSection ImportSec;
  FunctionSection FunctionSec;
  TableSection TableSec;
  MemorySection MemorySec;
  GlobalSection GlobalSec;
  ExportSection ExportSec;
  StartSection StartSec;
  ElementSection ElementSec;
  CodeSection CodeSec;
  DataSection DataSec;
  DataCountSection DataCountSec;
  /// @}
  AOTSection AOTSec;

  Symbol<const IntrinsicsTable *> IntrSymbol;
};

} // namespace AST
} // namespace WasmEdge
