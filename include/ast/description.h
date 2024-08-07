// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/description.h - Desc classes definitions -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Desc node class and the derived
/// ImportDesc and ExportDesc classes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "common/enum_types.hpp"

#include <string>
#include <string_view>
#include <variant>

namespace WasmEdge {
namespace AST {

/// Base class of Desc node.
class Desc {
public:
  /// Getter and setter of external type.
  ExternalType getExternalType() const noexcept { return ExtType; }
  void setExternalType(ExternalType ET) noexcept { ExtType = ET; }

  /// Getter and setter of external name.
  std::string_view getExternalName() const noexcept { return ExtName; }
  void setExternalName(std::string_view Name) { ExtName = Name; }

protected:
  /// \name Data of Desc: rxternal name and external type.
  /// @{
  ExternalType ExtType;
  std::string ExtName;
  /// @}
};

/// Derived import description class.
class ImportDesc : public Desc {
public:
  /// Getter and setter of module name.
  std::string_view getModuleName() const noexcept { return ModName; }
  void setModuleName(std::string_view Name) { ModName = Name; }

  /// Getter and setter of external contents.
  uint32_t getExternalFuncTypeIdx() const noexcept { return FuncTypeIdx; }
  void setExternalFuncTypeIdx(uint32_t Idx) noexcept { FuncTypeIdx = Idx; }
  const TableType &getExternalTableType() const noexcept { return TabType; }
  TableType &getExternalTableType() noexcept { return TabType; }
  const MemoryType &getExternalMemoryType() const noexcept { return MemType; }
  MemoryType &getExternalMemoryType() noexcept { return MemType; }
  const GlobalType &getExternalGlobalType() const noexcept { return GlobType; }
  GlobalType &getExternalGlobalType() noexcept { return GlobType; }
  const TagType &getExternalTagType() const noexcept { return TgType; }
  TagType &getExternalTagType() noexcept { return TgType; }

private:
  /// \name Data of ImportDesc: Module name, External name, and content node.
  /// @{
  std::string ModName;
  uint32_t FuncTypeIdx = 0;
  TableType TabType;
  MemoryType MemType;
  GlobalType GlobType;
  TagType TgType;
  /// @}
};

/// Derived export description class.
class ExportDesc : public Desc {
public:
  /// Getter and setter of external index.
  uint32_t getExternalIndex() const noexcept { return ExtIdx; }
  void setExternalIndex(uint32_t Idx) noexcept { ExtIdx = Idx; }

private:
  /// \name Data of ExportDesc: external index.
  /// @{
  uint32_t ExtIdx;
  /// @}
};

} // namespace AST
} // namespace WasmEdge
