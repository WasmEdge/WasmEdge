// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/ast/description.h - Desc classes definition --------------===//
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

#include "ast/base.h"
#include "ast/type.h"

#include <string>
#include <variant>

namespace WasmEdge {
namespace AST {

/// Base class of Desc node.
class Desc : public Base {
public:
  /// Getter of external type.
  virtual ExternalType getExternalType() const { return ExtType; }

protected:
  /// External type of this class.
  ExternalType ExtType;
};

/// Derived import description class.
class ImportDesc : public Desc {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read the module name, external name, external type, and corresponding
  /// content.
  ///
  /// \param Mgr the file manager reference.
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

  /// Getter of module name.
  std::string_view getModuleName() const { return ModName; }

  /// Getter of external name.
  std::string_view getExternalName() const { return ExtName; }

  /// Getter of external contents.
  uint32_t getExternalFuncTypeIdx() const { return FuncTypeIdx; }
  const TableType &getExternalTableType() const { return TabType; }
  const MemoryType &getExternalMemoryType() const { return MemType; }
  const GlobalType &getExternalGlobalType() const { return GlobType; }

  /// The node type should be ASTNodeAttr::Desc_Import.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Desc_Import;

private:
  /// \name Data of ImportDesc: Module name, External name, and content node.
  /// @{
  std::string ModName;
  std::string ExtName;
  uint32_t FuncTypeIdx = 0;
  TableType TabType;
  MemoryType MemType;
  GlobalType GlobType;
  /// @}
};

class ExportDesc : public Desc {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read the export name, external type, and corresponding external index.
  ///
  /// \param Mgr the file manager reference.
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

  /// Getter of external name.
  std::string_view getExternalName() const { return ExtName; }

  /// Getter of external index.
  uint32_t getExternalIndex() const { return ExtIdx; }

  /// The node type should be ASTNodeAttr::Desc_Export.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Desc_Export;

private:
  /// \name Data of ExportDesc: External name and external index.
  /// @{
  std::string ExtName;
  uint32_t ExtIdx;
  /// @}
};

} // namespace AST
} // namespace WasmEdge
