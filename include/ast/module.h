// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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

#include "ast/section.h"

#include <vector>

namespace WasmEdge {
namespace AST {

/// AST Module node.
class Module {
public:
  /// Getter of magic vector.
  const std::vector<Byte> &getMagic() const noexcept { return Magic; }
  std::vector<Byte> &getMagic() noexcept { return Magic; }

  /// Getter of version vector.
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
  const AOTSection &getAOTSection() const { return AOTSec; }
  AOTSection &getAOTSection() { return AOTSec; }

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
    kPtrFunc,
    kMemoryAtomicNotify,
    kMemoryAtomicWait,
    kIntrinsicMax,
  };
  using IntrinsicsTable = void * [uint32_t(Intrinsics::kIntrinsicMax)];

  /// Getter and setter of compiled symbol.
  const auto &getSymbol() const noexcept { return IntrSymbol; }
  void setSymbol(Symbol<const IntrinsicsTable *> S) noexcept {
    IntrSymbol = std::move(S);
  }

  /// Getter and setter of validated flag.
  bool getIsValidated() const noexcept { return IsValidated; }
  void setIsValidated(bool V = true) noexcept { IsValidated = V; }

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

  /// \name Data of AOT.
  /// @{
  AOTSection AOTSec;
  Symbol<const IntrinsicsTable *> IntrSymbol;
  /// @}

  /// \name Validated flag.
  /// @{
  bool IsValidated = false;
  /// @}
};

} // namespace AST
} // namespace WasmEdge
