//===-- ssvm/ast/type.h - type classes definition ---------------*- C++ -*-===//
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
#include "executor/storemgr.h"

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

  /// Instantiate to store manager.
  ///
  /// Set the min and max value to instance.
  ///
  /// \param Mgr the store manager reference.
  /// \param Instance the table/memory instance reference.
  ///
  /// \returns ErrCode.
  template <typename T>
  Executor::ErrCode instantiate(StoreMgr &Mgr, std::unique_ptr<T> &Instance);

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

  /// Instantiate to store manager.
  ///
  /// Insert the vector of param types and return types to Module instance.
  ///
  /// \param Mgr the store manager reference.
  /// \param ModInstId the index of module instance in store manager.
  ///
  /// \returns ErrCode.
  Executor::ErrCode instantiate(StoreMgr &Mgr, unsigned int ModInstId);

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

  /// Instantiate to store manager.
  ///
  /// Set the limit to table instance.
  ///
  /// \param Mgr the store manager reference.
  /// \param MemInst the reference to memory instance pointer.
  ///
  /// \returns ErrCode.
  Executor::ErrCode instantiate(StoreMgr &Mgr,
                                std::unique_ptr<MemoryInstance> &MemInst);

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

  /// Instantiate to store manager.
  ///
  /// Set the element type and limit to table instance.
  ///
  /// \param Mgr the store manager reference.
  /// \param TabInst the reference to table instance pointer.
  ///
  /// \returns ErrCode.
  Executor::ErrCode instantiate(StoreMgr &Mgr,
                                std::unique_ptr<TableInstance> &TabInst);

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

  /// Instantiate to store manager.
  ///
  /// Set the value type and mutibility to global instance.
  ///
  /// \param Mgr the store manager reference.
  /// \param GlobInst the reference to global instance pointer.
  ///
  /// \returns ErrCode.
  Executor::ErrCode instantiate(StoreMgr &Mgr,
                                std::unique_ptr<GlobalInstance> &GlobInst);

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
