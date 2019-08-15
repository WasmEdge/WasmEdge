//===-- ssvm/ast/description.h - Desc classes definition---------*- C++ -*-===//
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

#include "type.h"
#include <string>
#include <variant>

namespace SSVM {
namespace AST {

/// Base class of Desc node.
class Desc : public Base {
public:
  /// External type enumeration class
  enum class ExternalType : unsigned char {
    Function = 0x00U,
    Table = 0x01U,
    Memory = 0x02U,
    Global = 0x03U
  };

  /// Getter of external type.
  virtual ExternalType getExternalType() { return ExtType; }

protected:
  /// External type of this class.
  ExternalType ExtType;
};

/// Derived import description class.
class ImportDesc : public Desc {
public:
  /// Variant of external type classes.
  using ExtContentType =
      std::variant<std::unique_ptr<unsigned int>, std::unique_ptr<TableType>,
                   std::unique_ptr<MemoryType>, std::unique_ptr<GlobalType>>;

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read the module name, external name, external type, and corresponding
  /// content.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

  /// Getter of module name.
  const std::string &getModuleName() const { return ModName; }

  /// Getter of external name.
  const std::string &getExternalName() const { return ExtName; }

  /// Getter of ExtContent.
  template <typename T> Executor::ErrCode getExternalContent(T *&type) {
    try {
      type = std::get<std::unique_ptr<T>>(ExtContent).get();
    } catch (std::bad_variant_access E) {
      return Executor::ErrCode::TypeNotMatch;
    }
    return Executor::ErrCode::Success;
  }

protected:
  /// The node type should be Attr::Desc_Import.
  Attr NodeAttr = Attr::Desc_Import;

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
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

  /// Getter of external name.
  const std::string &getExternalName() { return ExtName; }

  /// Getter of external index.
  unsigned int getExternalIndex() { return ExtIdx; }

protected:
  /// The node type should be Attr::Desc_Export.
  Attr NodeAttr = Attr::Desc_Export;

private:
  /// \name Data of ExportDesc: External name and external index.
  /// @{
  std::string ExtName;
  unsigned int ExtIdx;
  /// @}
};

} // namespace AST
} // namespace SSVM
