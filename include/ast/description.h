// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/ast/description.h - Desc classes definition ------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Desc node class and the derived
/// ImportDesc and ExportDesc classes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/types.h"
#include "type.h"

#include <string>
#include <variant>

namespace SSVM {
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
  /// Variant of external type classes.
  using ExtContentType =
      std::variant<std::unique_ptr<uint32_t>, std::unique_ptr<TableType>,
                   std::unique_ptr<MemoryType>, std::unique_ptr<GlobalType>>;

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read the module name, external name, external type, and corresponding
  /// content.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of module name.
  std::string_view getModuleName() const { return ModName; }

  /// Getter of external name.
  std::string_view getExternalName() const { return ExtName; }

  /// Getter of ExtContent.
  template <typename T> Expect<T *> getExternalContent() const {
    if (auto Ptr = std::get_if<std::unique_ptr<T>>(&ExtContent)) {
      return Ptr->get();
    } else {
      return Unexpect(ErrCode::IncompatibleImportType);
    }
  }

  /// The node type should be ASTNodeAttr::Desc_Import.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Desc_Import;

private:
  /// \name Data of ImportDesc: Module name, External name, and content node.
  /// @{
  std::string ModName;
  std::string ExtName;
  ExtContentType ExtContent;
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
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

  /// Getter of external name.
  std::string_view getExternalName() const { return ExtName; }

  /// Getter of external index.
  uint32_t getExternalIndex() const { return ExtIdx; }

  /// The node type should be ASTNodeAttr::Desc_Export.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Desc_Export;

private:
  /// \name Data of ExportDesc: External name and external index.
  /// @{
  std::string ExtName;
  uint32_t ExtIdx;
  /// @}
};

} // namespace AST
} // namespace SSVM
