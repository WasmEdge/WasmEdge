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
#include "loader/symbol.h"
#include "section.h"

#include <memory>
#include <string>
#include <string_view>
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
  /// \returns void when success, ErrCode when failed.
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

  using TrapCodeProxy = uint32_t *;
  using CallProxy = void (*)(const uint32_t FuncIdx, const ValVariant *Args,
                             ValVariant *Rets);
  using MemGrowProxy = uint32_t (*)(const uint32_t Diff);

  void setTrapCodeProxySymbol(DLSymbol<TrapCodeProxy> Symbol) {
    TrapCodeProxySymbol = std::move(Symbol);
  }
  void setCallProxySymbol(DLSymbol<CallProxy> Symbol) {
    CallProxySymbol = std::move(Symbol);
  }
  void setMemGrowProxySymbol(DLSymbol<MemGrowProxy> Symbol) {
    MemGrowProxySymbol = std::move(Symbol);
  }
  void setTrapCodeProxy(TrapCodeProxy Pointer) const {
    if (TrapCodeProxySymbol)
      *TrapCodeProxySymbol = Pointer;
  }
  void setCallProxy(CallProxy Pointer) const {
    if (CallProxySymbol)
      *CallProxySymbol = Pointer;
  }
  void setMemGrowProxy(MemGrowProxy Pointer) const {
    if (MemGrowProxySymbol)
      *MemGrowProxySymbol = Pointer;
  }

  /// The node type should be ASTNodeAttr::Module.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Module;

  /// Mangling outstanding names for ELF symbol name.
  /// `$` -> `$$`
  /// `#` -> `$1`
  /// `@` -> `$2`
  /// `\0` -> `$0`
  static std::string toExportName(std::string_view Name) {
    using namespace std::literals::string_view_literals;
    const auto NPos = std::string_view::npos;
    const auto Esc = "$#@\0"sv;
    std::string Result(1, '$');
    size_t Cursor = 0;
    for (size_t Pos = Name.find_first_of(Esc); Pos != NPos;
         Cursor = Pos + 1, Pos = Name.find_first_of(Esc, Cursor)) {
      Result += Name.substr(Cursor, Pos - Cursor);
      switch (Name[Pos]) {
      case '$':
        Result += '$';
        Result += '$';
        break;
      case '#':
        Result += '$';
        Result += '1';
        break;
      case '@':
        Result += '$';
        Result += '2';
        break;
      case '\0':
        Result += '$';
        Result += '0';
        break;
      }
    }
    Result += Name.substr(Cursor);
    return Result;
  }

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

  DLSymbol<TrapCodeProxy> TrapCodeProxySymbol;
  DLSymbol<CallProxy> CallProxySymbol;
  DLSymbol<MemGrowProxy> MemGrowProxySymbol;
};

} // namespace AST
} // namespace SSVM
