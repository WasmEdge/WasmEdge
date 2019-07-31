//===-- ssvm/ast/section.h - Section class definition -----------*- C++ -*-===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Section node class, which is the
/// module node in AST, and the subsection classes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "description.h"
#include "segment.h"
#include "type.h"

namespace AST {

/// Section's base class.
class Section : public Base {
public:
  /// Load binary from file manager.
  ///
  /// Inheritted and overrided from Base.
  /// Read the content size of this section.
  /// Call loadContent() for reading contents.
  ///
  /// \param Mgr the file manager reference.
  ///
  /// \returns ErrCode.
  virtual Loader::ErrCode loadBinary(FileMgr &Mgr);

protected:
  /// Read content size of this section.
  Loader::ErrCode loadSize(FileMgr &Mgr);

  /// Read content of this section.
  virtual Loader::ErrCode loadContent(FileMgr &Mgr) {
    return Loader::ErrCode::InvalidGrammar;
  };

  /// Template function of reading vector of type T.
  ///
  /// Helper function of read variaties of vectors.
  /// Read the content size of this section.
  /// Call loadContent() for reading contents.
  ///
  /// \param Mgr the file manager reference.
  /// \param [out] &Vec filled with read data on loadVector success.
  ///
  /// \returns ErrCode.
  template <typename T>
  Loader::ErrCode loadVector(FileMgr &Mgr,
                             std::vector<std::unique_ptr<T>> &Vec);

  /// Content size of this section.
  unsigned int ContentSize = 0;
};

/// AST CustomSection node.
class CustomSection : public Section {
protected:
  /// Overrided content loading of custom section.
  virtual Loader::ErrCode loadContent(FileMgr &Mgr);

  /// The node type should be Attr::Sec_Custom.
  Attr NodeAttr = Attr::Sec_Custom;

private:
  /// Vector of raw bytes of content.
  std::vector<unsigned char> Content;
};

/// AST TypeSection node.
class TypeSection : public Section {
public:
  /// Instantiate to store manager.
  ///
  /// Move the vector of function types to Module instance.
  ///
  /// \param Mgr the store manager reference.
  /// \param ModInstId the index of module instance in store manager.
  ///
  /// \returns ErrCode.
  Executor::ErrCode instantiate(StoreMgr &Mgr, unsigned int ModInstId);

protected:
  /// Overrided content loading of type section.
  virtual Loader::ErrCode loadContent(FileMgr &Mgr);

  /// The node type should be Attr::Sec_Type.
  Attr NodeAttr = Attr::Sec_Type;

private:
  /// Vector of FunctionType nodes.
  std::vector<std::unique_ptr<FunctionType>> Content;
};

/// AST ImportSection node.
class ImportSection : public Section {
protected:
  /// Overrided content loading of import section.
  virtual Loader::ErrCode loadContent(FileMgr &Mgr);

  /// The node type should be Attr::Sec_Import.
  Attr NodeAttr = Attr::Sec_Import;

private:
  /// Vector of ImportDesc nodes.
  std::vector<std::unique_ptr<ImportDesc>> Content;
};

/// AST FunctionSection node.
class FunctionSection : public Section {
public:
  /// Instantiate to store manager.
  ///
  /// Make function instances and move expressions to instances.
  ///
  /// \param Mgr the store manager reference.
  /// \param TypeIdx the type indices list for output.
  ///
  /// \returns ErrCode.
  Executor::ErrCode instantiate(StoreMgr &Mgr,
                                std::vector<unsigned int> &TypeIdx);

protected:
  /// Overrided content loading of function section.
  virtual Loader::ErrCode loadContent(FileMgr &Mgr);

  /// The node type should be Attr::Sec_Function.
  Attr NodeAttr = Attr::Sec_Function;

private:
  /// Vector of function indices.
  std::vector<unsigned int> Content;
};

/// AST TableSection node.
class TableSection : public Section {
public:
  /// Instantiate to store manager.
  ///
  /// Move the vector of table types to instances.
  ///
  /// \param Mgr the store manager reference.
  /// \param ModInstId the index of module instance in store manager.
  ///
  /// \returns ErrCode.
  Executor::ErrCode instantiate(StoreMgr &Mgr, unsigned int ModInstId);

protected:
  /// Overrided content loading of table section.
  virtual Loader::ErrCode loadContent(FileMgr &Mgr);

  /// The node type should be Attr::Sec_Table.
  Attr NodeAttr = Attr::Sec_Table;

private:
  /// Vector of TableType nodes.
  std::vector<std::unique_ptr<TableType>> Content;
};

/// AST MemorySection node.
class MemorySection : public Section {
public:
  /// Instantiate to store manager.
  ///
  /// Move the vector of memory types to instances.
  ///
  /// \param Mgr the store manager reference.
  /// \param ModInstId the index of module instance in store manager.
  ///
  /// \returns ErrCode.
  Executor::ErrCode instantiate(StoreMgr &Mgr, unsigned int ModInstId);

protected:
  /// Overrided content loading of memory section.
  virtual Loader::ErrCode loadContent(FileMgr &Mgr);

  /// The node type should be Attr::Sec_Memory.
  Attr NodeAttr = Attr::Sec_Memory;

private:
  /// Vector of MemoryType nodes.
  std::vector<std::unique_ptr<MemoryType>> Content;
};

/// AST GlobalSection node.
class GlobalSection : public Section {
public:
  /// Instantiate to store manager.
  ///
  /// Make global instances and move expressions to instances.
  ///
  /// \param Mgr the store manager reference.
  /// \param ModInstId the index of module instance in store manager.
  ///
  /// \returns ErrCode.
  Executor::ErrCode instantiate(StoreMgr &Mgr, unsigned int ModInstId);

protected:
  /// Overrided content loading of global section.
  virtual Loader::ErrCode loadContent(FileMgr &Mgr);

  /// The node type should be Attr::Sec_Global.
  Attr NodeAttr = Attr::Sec_Global;

private:
  /// Vector of GlobalType nodes.
  std::vector<std::unique_ptr<GlobalSegment>> Content;
};

/// AST ExportSection node.
class ExportSection : public Section {
protected:
  /// Overrided content loading of export section.
  virtual Loader::ErrCode loadContent(FileMgr &Mgr);

  /// The node type should be Attr::Sec_Export.
  Attr NodeAttr = Attr::Sec_Export;

private:
  /// Vector of ExportDesc nodes.
  std::vector<std::unique_ptr<ExportDesc>> Content;
};

/// AST StartSection node.
class StartSection : public Section {
protected:
  /// Overrided content loading of start section.
  virtual Loader::ErrCode loadContent(FileMgr &Mgr);

  /// The node type should be Attr::Sec_Start.
  Attr NodeAttr = Attr::Sec_Start;

private:
  /// Start function index.
  unsigned int Content;
};

/// AST ElementSection node.
class ElementSection : public Section {
protected:
  /// Overrided content loading of element section.
  virtual Loader::ErrCode loadContent(FileMgr &Mgr);

  /// The node type should be Attr::Sec_Element.
  Attr NodeAttr = Attr::Sec_Element;

private:
  /// Vector of ElementSegment nodes.
  std::vector<std::unique_ptr<ElementSegment>> Content;
};

/// AST CodeSection node.
class CodeSection : public Section {
public:
  /// Instantiate to store manager.
  ///
  /// Make function instances and move Code Segment to instances.
  ///
  /// \param Mgr the store manager reference.
  /// \param ModInstId the index of module instance in store manager.
  /// \param TypeSec the corresponding function section for getting type index.
  ///
  /// \returns ErrCode.
  Executor::ErrCode instantiate(StoreMgr &Mgr, unsigned int ModInstId,
                                std::unique_ptr<AST::FunctionSection> &FuncSec);

protected:
  /// Overrided content loading of code section.
  virtual Loader::ErrCode loadContent(FileMgr &Mgr);

  /// The node type should be Attr::Sec_Code.
  Attr NodeAttr = Attr::Sec_Code;

private:
  /// Vector of CodeSegment nodes.
  std::vector<std::unique_ptr<CodeSegment>> Content;
};

/// AST DataSection node.
class DataSection : public Section {
protected:
  /// Overrided content loading of data section.
  virtual Loader::ErrCode loadContent(FileMgr &Mgr);

  /// The node type should be Attr::Sec_Data.
  Attr NodeAttr = Attr::Sec_Data;

private:
  /// Vector of DataSegment nodes.
  std::vector<std::unique_ptr<DataSegment>> Content;
};

} // namespace AST
