// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/ast/module.h - Module class definition -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Module node class, which is the
/// module node in the AST.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/section.h"

#include <utility>
#include <vector>

namespace WasmEdge {
namespace AST {

/// AST Module node.
class Module {
public:
  Module() = default;

  /// Copy constructor. The tag types of the copy point to its own types.
  Module(const Module &Mod)
      : Magic(Mod.Magic), Version(Mod.Version), CustomSecs(Mod.CustomSecs),
        TypeSec(Mod.TypeSec), ImportSec(Mod.ImportSec),
        FunctionSec(Mod.FunctionSec), TableSec(Mod.TableSec),
        MemorySec(Mod.MemorySec), GlobalSec(Mod.GlobalSec),
        ExportSec(Mod.ExportSec), StartSec(Mod.StartSec),
        ElementSec(Mod.ElementSec), CodeSec(Mod.CodeSec), DataSec(Mod.DataSec),
        DataCountSec(Mod.DataCountSec), TagSec(Mod.TagSec), AOTSec(Mod.AOTSec),
        IntrSymbol(Mod.IntrSymbol), IsValidated(Mod.IsValidated) {
    auto &TypeVec = TypeSec.getContent();
    for (auto &TgType : TagSec.getContent()) {
      if (TgType.getTypeIdx() < TypeVec.size()) {
        TgType.setDefType(&TypeVec[TgType.getTypeIdx()]);
      }
    }
    for (auto &Desc : ImportSec.getContent()) {
      if (Desc.getExternalType() == ExternalType::Tag) {
        auto &TgType = Desc.getExternalTagType();
        if (TgType.getTypeIdx() < TypeVec.size()) {
          TgType.setDefType(&TypeVec[TgType.getTypeIdx()]);
        }
      }
    }
  }

  /// Move constructor.
  Module(Module &&Mod) = default;

  /// Copy assignment.
  Module &operator=(const Module &Mod) {
    if (this != &Mod) {
      Module Tmp(Mod);
      *this = std::move(Tmp);
    }
    return *this;
  }

  /// Move assignment.
  Module &operator=(Module &&Mod) = default;

  /// Getter for magic vector.
  const std::vector<Byte> &getMagic() const noexcept { return Magic; }
  std::vector<Byte> &getMagic() noexcept { return Magic; }

  /// Getter for version vector.
  const std::vector<Byte> &getVersion() const noexcept { return Version; }
  std::vector<Byte> &getVersion() noexcept { return Version; }

  /// Getters of references to sections.
  Span<const CustomSection> getCustomSections() const noexcept {
    return CustomSecs;
  }
  std::vector<CustomSection> &getCustomSections() noexcept {
    return CustomSecs;
  }
  const TypeSection &getTypeSection() const { return TypeSec; }
  TypeSection &getTypeSection() { return TypeSec; }
  const ImportSection &getImportSection() const { return ImportSec; }
  ImportSection &getImportSection() { return ImportSec; }
  const FunctionSection &getFunctionSection() const { return FunctionSec; }
  FunctionSection &getFunctionSection() { return FunctionSec; }
  const TableSection &getTableSection() const { return TableSec; }
  TableSection &getTableSection() { return TableSec; }
  const MemorySection &getMemorySection() const { return MemorySec; }
  MemorySection &getMemorySection() { return MemorySec; }
  const GlobalSection &getGlobalSection() const { return GlobalSec; }
  GlobalSection &getGlobalSection() { return GlobalSec; }
  const ExportSection &getExportSection() const { return ExportSec; }
  ExportSection &getExportSection() { return ExportSec; }
  const StartSection &getStartSection() const { return StartSec; }
  StartSection &getStartSection() { return StartSec; }
  const ElementSection &getElementSection() const { return ElementSec; }
  ElementSection &getElementSection() { return ElementSec; }
  const CodeSection &getCodeSection() const { return CodeSec; }
  CodeSection &getCodeSection() { return CodeSec; }
  const DataSection &getDataSection() const { return DataSec; }
  DataSection &getDataSection() { return DataSec; }
  const DataCountSection &getDataCountSection() const { return DataCountSec; }
  DataCountSection &getDataCountSection() { return DataCountSec; }
  const TagSection &getTagSection() const { return TagSec; }
  TagSection &getTagSection() { return TagSec; }
  const AOTSection &getAOTSection() const { return AOTSec; }
  AOTSection &getAOTSection() { return AOTSec; }

  /// Get the number of imported functions.
  uint32_t getImportFuncCount() const noexcept {
    uint32_t Count = 0;
    for (const auto &ImpDesc : ImportSec.getContent()) {
      if (ImpDesc.getExternalType() == ExternalType::Function) {
        ++Count;
      }
    }
    return Count;
  }

  /// Get the number of defined (non-imported) functions.
  uint32_t getDefinedFuncCount() const noexcept {
    return static_cast<uint32_t>(CodeSec.getContent().size());
  }

  /// Getter and setter for compiled symbol.
  const auto &getSymbol() const noexcept { return IntrSymbol; }
  void setSymbol(Symbol<const Executable::IntrinsicsTable *> S) noexcept {
    IntrSymbol = std::move(S);
  }

  /// Getter and setter for validated flag.
  bool getIsValidated() const noexcept { return IsValidated; }
  void setIsValidated(bool V = true) const noexcept { IsValidated = V; }

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
  TagSection TagSec;
  /// @}

  /// \name Data of AOT.
  /// @{
  AOTSection AOTSec;
  Symbol<const Executable::IntrinsicsTable *> IntrSymbol;
  /// @}

  /// \name Validated flag.
  /// @{
  mutable bool IsValidated = false;
  /// @}
};

} // namespace AST
} // namespace WasmEdge
