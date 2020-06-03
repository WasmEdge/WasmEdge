// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/ast/section.h - Section class definition --------------===//
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

#include "base.h"
#include "description.h"
#include "segment.h"
#include "type.h"

#include <memory>
#include <vector>

namespace SSVM {
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
  /// \returns void when success, ErrMsg when failed.
  Expect<void> loadBinary(FileMgr &Mgr) override;

protected:
  /// Read content size of this section.
  Expect<void> loadSize(FileMgr &Mgr);

  /// Read content of this section.
  virtual Expect<void> loadContent(FileMgr &Mgr) {
    return Unexpect(ErrCode::InvalidGrammar);
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
  /// \returns void when success, ErrMsg when failed.
  template <typename T>
  Expect<void> loadToVector(FileMgr &Mgr,
                            std::vector<std::unique_ptr<T>> &Vec) {
    uint32_t VecCnt = 0;
    /// Read vector size.
    if (auto Res = Mgr.readU32()) {
      VecCnt = *Res;
    } else {
      return Unexpect(Res);
    }

    /// Sequently create AST node T and read data.
    for (uint32_t i = 0; i < VecCnt; ++i) {
      auto NewContent = std::make_unique<T>();
      if (auto Res = NewContent->loadBinary(Mgr)) {
        Vec.push_back(std::move(NewContent));
      } else {
        return Unexpect(Res);
      }
    }
    return {};
  }

  /// Content size of this section.
  uint32_t ContentSize = 0;
};

/// AST CustomSection node.
class CustomSection : public Section {
protected:
  /// Overrided content loading of custom section.
  Expect<void> loadContent(FileMgr &Mgr) override;

  /// The node type should be ASTNodeAttr::Sec_Custom.
  ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Custom;

private:
  /// Vector of raw bytes of content.
  std::vector<Byte> Content;
};

/// AST TypeSection node.
class TypeSection : public Section {
public:
  /// Getter of content vector.
  Span<const std::unique_ptr<FunctionType>> getContent() const {
    return Content;
  }

protected:
  /// Overrided content loading of type section.
  Expect<void> loadContent(FileMgr &Mgr) override;

  /// The node type should be ASTNodeAttr::Sec_Type.
  ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Type;

private:
  /// Vector of FunctionType nodes.
  std::vector<std::unique_ptr<FunctionType>> Content;
};

/// AST ImportSection node.
class ImportSection : public Section {
public:
  /// Getter of content vector.
  Span<const std::unique_ptr<ImportDesc>> getContent() const { return Content; }

protected:
  /// Overrided content loading of import section.
  Expect<void> loadContent(FileMgr &Mgr) override;

  /// The node type should be ASTNodeAttr::Sec_Import.
  ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Import;

private:
  /// Vector of ImportDesc nodes.
  std::vector<std::unique_ptr<ImportDesc>> Content;
};

/// AST FunctionSection node.
class FunctionSection : public Section {
public:
  /// Getter of content vector.
  Span<const uint32_t> getContent() const { return Content; }

protected:
  /// Overrided content loading of function section.
  Expect<void> loadContent(FileMgr &Mgr) override;

  /// The node type should be ASTNodeAttr::Sec_Function.
  ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Function;

private:
  /// Vector of function indices.
  std::vector<uint32_t> Content;
};

/// AST TableSection node.
class TableSection : public Section {
public:
  /// Getter of content vector.
  Span<const std::unique_ptr<TableType>> getContent() const { return Content; }

protected:
  /// Overrided content loading of table section.
  Expect<void> loadContent(FileMgr &Mgr) override;

  /// The node type should be ASTNodeAttr::Sec_Table.
  ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Table;

private:
  /// Vector of TableType nodes.
  std::vector<std::unique_ptr<TableType>> Content;
};

/// AST MemorySection node.
class MemorySection : public Section {
public:
  /// Getter of content vector.
  Span<const std::unique_ptr<MemoryType>> getContent() const { return Content; }

protected:
  /// Overrided content loading of memory section.
  Expect<void> loadContent(FileMgr &Mgr) override;

  /// The node type should be ASTNodeAttr::Sec_Memory.
  ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Memory;

private:
  /// Vector of MemoryType nodes.
  std::vector<std::unique_ptr<MemoryType>> Content;
};

/// AST GlobalSection node.
class GlobalSection : public Section {
public:
  /// Getter of content vector.
  Span<const std::unique_ptr<GlobalSegment>> getContent() const {
    return Content;
  }

protected:
  /// Overrided content loading of global section.
  virtual Expect<void> loadContent(FileMgr &Mgr);

  /// The node type should be ASTNodeAttr::Sec_Global.
  ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Global;

private:
  /// Vector of GlobalType nodes.
  std::vector<std::unique_ptr<GlobalSegment>> Content;
};

/// AST ExportSection node.
class ExportSection : public Section {
public:
  /// Getter of content vector.
  Span<const std::unique_ptr<ExportDesc>> getContent() const { return Content; }

protected:
  /// Overrided content loading of export section.
  virtual Expect<void> loadContent(FileMgr &Mgr);

  /// The node type should be ASTNodeAttr::Sec_Export.
  ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Export;

private:
  /// Vector of ExportDesc nodes.
  std::vector<std::unique_ptr<ExportDesc>> Content;
};

/// AST StartSection node.
class StartSection : public Section {
public:
  /// Getter of content.
  uint32_t getContent() const { return Content; }

protected:
  /// Overrided content loading of start section.
  virtual Expect<void> loadContent(FileMgr &Mgr);

  /// The node type should be ASTNodeAttr::Sec_Start.
  ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Start;

private:
  /// Start function index.
  uint32_t Content;
};

/// AST ElementSection node.
class ElementSection : public Section {
public:
  /// Getter of content vector.
  Span<const std::unique_ptr<ElementSegment>> getContent() const {
    return Content;
  }

protected:
  /// Overrided content loading of element section.
  virtual Expect<void> loadContent(FileMgr &Mgr);

  /// The node type should be ASTNodeAttr::Sec_Element.
  ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Element;

private:
  /// Vector of ElementSegment nodes.
  std::vector<std::unique_ptr<ElementSegment>> Content;
};

/// AST CodeSection node.
class CodeSection : public Section {
public:
  /// Getter of content vector.
  Span<const std::unique_ptr<CodeSegment>> getContent() const {
    return Content;
  }

protected:
  /// Overrided content loading of code section.
  virtual Expect<void> loadContent(FileMgr &Mgr);

  /// The node type should be ASTNodeAttr::Sec_Code.
  ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Code;

private:
  /// Vector of CodeSegment nodes.
  std::vector<std::unique_ptr<CodeSegment>> Content;
};

/// AST DataSection node.
class DataSection : public Section {
public:
  /// Getter of content vector.
  Span<const std::unique_ptr<DataSegment>> getContent() const {
    return Content;
  }

protected:
  /// Overrided content loading of data section.
  virtual Expect<void> loadContent(FileMgr &Mgr);

  /// The node type should be ASTNodeAttr::Sec_Data.
  ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Data;

private:
  /// Vector of DataSegment nodes.
  std::vector<std::unique_ptr<DataSegment>> Content;
};

} // namespace AST
} // namespace SSVM
