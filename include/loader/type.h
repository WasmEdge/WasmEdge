//===-- ssvm/loader/type.h - type classes definition ------------*- C++ -*-===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the type classes: Limit, FunctionType,
/// MemoryType, TableType, and GlobalType.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "base.h"

#include <memory>

namespace AST {

/// AST Limit node.
class Limit : public Base {
public:
  /// Limit type enumeration class.
  enum class LimitType : unsigned char { HasMin = 0x00, HasMinMax = 0x01 };

  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read limit type.
  /// Read Min and Max value of this node.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

protected:
  /// The node type should be Attr::Type_Limit.
  Attr NodeAttr = Attr::Type_Limit;

private:
  /// \name Data of Limit node.
  /// @{
  LimitType Type;
  unsigned int Min;
  unsigned int Max;
  /// @}
};

/// AST FunctionType node.
class FunctionType : public Base {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read value types of parameter list and return list.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

protected:
  /// The node type should be Attr::Type_Function.
  Attr NodeAttr = Attr::Type_Function;

private:
  /// \name Data of FunctionType node.
  /// @{
  std::vector<ValType> ParamTypes;
  std::vector<ValType> ReturnTypes;
  /// @}
};

/// AST MemoryType node.
class MemoryType : public Base {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read the Limit data of this node.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

protected:
  /// The node type should be Attr::Type_Memory.
  Attr NodeAttr = Attr::Type_Memory;

private:
  /// Data of MemoryType node.
  std::unique_ptr<Limit> Memory;
};

/// AST TableType node.
class TableType : public Base {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read element type and Limit data.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

protected:
  /// The node type should be Attr::Type_Table.
  Attr NodeAttr = Attr::Type_Table;

private:
  /// \name Data of TableType node.
  /// @{
  ElemType Type;
  std::unique_ptr<Limit> Table;
  /// @}
};

/// AST GlobalType node.
class GlobalType : public Base {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read value type and mutation.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

protected:
  /// The node type should be Attr::Type_Global.
  Attr NodeAttr = Attr::Type_Global;

private:
  /// \name Data of GlobalType node.
  /// @{
  ValType Type;
  ValMut Mut;
  /// @}
};

} // namespace AST
