// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/ast/section.h - Section class definition ---------------------===//
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

#include <memory>
#include <vector>

#include "common/log.h"

#include "base.h"
#include "description.h"
#include "segment.h"
#include "type.h"

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
  /// \param PConf the proposal configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr,
                          const ProposalConfigure &PConf) override;

protected:
  /// Read content size of this section.
  Expect<void> loadSize(FileMgr &Mgr);

  /// Read content of this section.
  virtual Expect<void> loadContent(FileMgr &Mgr,
                                   const ProposalConfigure &PConf) {
    LOG(ERROR) << ErrCode::InvalidGrammar;
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(ErrCode::InvalidGrammar);
  };

  /// Template function of reading vector of type T.
  ///
  /// Helper function of read variaties of vectors.
  /// Read the content size of this section.
  /// Call loadContent() for reading contents.
  ///
  /// \param Mgr the file manager reference.
  /// \param PConf the proposal configuration reference.
  /// \param Node the node type of caller.
  /// \param [out] &Vec filled with read data on loadVector success.
  ///
  /// \returns void when success, ErrCode when failed.
  template <typename T>
  Expect<void> loadToVector(FileMgr &Mgr, const ProposalConfigure &PConf,
                            const ASTNodeAttr Node, std::vector<T> &Vec) {
    uint32_t VecCnt = 0;
    /// Read vector size.
    if (auto Res = Mgr.readU32()) {
      VecCnt = *Res;
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(), Node);
    }

    /// Sequently create AST node T and read data.
    for (uint32_t i = 0; i < VecCnt; ++i) {
      T NewContent;
      if (auto Res = NewContent.loadBinary(Mgr, PConf)) {
        Vec.push_back(std::move(NewContent));
      } else {
        LOG(ERROR) << ErrInfo::InfoAST(Node);
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
public:
  /// The node type should be ASTNodeAttr::Sec_Custom.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Custom;

protected:
  /// Overrided content loading of custom section.
  Expect<void> loadContent(FileMgr &Mgr,
                           const ProposalConfigure &PConf) override;

private:
  /// Vector of raw bytes of content.
  std::vector<Byte> Content;
};

/// AST TypeSection node.
class TypeSection : public Section {
public:
  /// Getter of content vector.
  Span<const FunctionType> getContent() const { return Content; }

  /// Getter of mutable content vector .
  std::vector<FunctionType> &getContent() { return Content; }

  /// The node type should be ASTNodeAttr::Sec_Type.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Type;

protected:
  /// Overrided content loading of type section.
  Expect<void> loadContent(FileMgr &Mgr,
                           const ProposalConfigure &PConf) override;

private:
  /// Vector of FunctionType nodes.
  std::vector<FunctionType> Content;
};

/// AST ImportSection node.
class ImportSection : public Section {
public:
  /// Getter of content vector.
  Span<const ImportDesc> getContent() const { return Content; }

  /// The node type should be ASTNodeAttr::Sec_Import.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Import;

protected:
  /// Overrided content loading of import section.
  Expect<void> loadContent(FileMgr &Mgr,
                           const ProposalConfigure &PConf) override;

private:
  /// Vector of ImportDesc nodes.
  std::vector<ImportDesc> Content;
};

/// AST FunctionSection node.
class FunctionSection : public Section {
public:
  /// Getter of content vector.
  Span<const uint32_t> getContent() const { return Content; }

  /// The node type should be ASTNodeAttr::Sec_Function.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Function;

protected:
  /// Overrided content loading of function section.
  Expect<void> loadContent(FileMgr &Mgr,
                           const ProposalConfigure &PConf) override;

private:
  /// Vector of function indices.
  std::vector<uint32_t> Content;
};

/// AST TableSection node.
class TableSection : public Section {
public:
  /// Getter of content vector.
  Span<const TableType> getContent() const { return Content; }

  /// The node type should be ASTNodeAttr::Sec_Table.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Table;

protected:
  /// Overrided content loading of table section.
  Expect<void> loadContent(FileMgr &Mgr,
                           const ProposalConfigure &PConf) override;

private:
  /// Vector of TableType nodes.
  std::vector<TableType> Content;
};

/// AST MemorySection node.
class MemorySection : public Section {
public:
  /// Getter of content vector.
  Span<const MemoryType> getContent() const { return Content; }

  /// The node type should be ASTNodeAttr::Sec_Memory.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Memory;

protected:
  /// Overrided content loading of memory section.
  Expect<void> loadContent(FileMgr &Mgr,
                           const ProposalConfigure &PConf) override;

private:
  /// Vector of MemoryType nodes.
  std::vector<MemoryType> Content;
};

/// AST GlobalSection node.
class GlobalSection : public Section {
public:
  /// Getter of content vector.
  Span<const GlobalSegment> getContent() const { return Content; }

  /// The node type should be ASTNodeAttr::Sec_Global.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Global;

protected:
  /// Overrided content loading of global section.
  Expect<void> loadContent(FileMgr &Mgr,
                           const ProposalConfigure &PConf) override;

private:
  /// Vector of GlobalType nodes.
  std::vector<GlobalSegment> Content;
};

/// AST ExportSection node.
class ExportSection : public Section {
public:
  /// Getter of content vector.
  Span<const ExportDesc> getContent() const { return Content; }

  /// The node type should be ASTNodeAttr::Sec_Export.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Export;

protected:
  /// Overrided content loading of export section.
  Expect<void> loadContent(FileMgr &Mgr,
                           const ProposalConfigure &PConf) override;

private:
  /// Vector of ExportDesc nodes.
  std::vector<ExportDesc> Content;
};

/// AST StartSection node.
class StartSection : public Section {
public:
  /// Getter of content.
  uint32_t getContent() const { return Content; }

  /// The node type should be ASTNodeAttr::Sec_Start.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Start;

protected:
  /// Overrided content loading of start section.
  Expect<void> loadContent(FileMgr &Mgr,
                           const ProposalConfigure &PConf) override;

private:
  /// Start function index.
  uint32_t Content;
};

/// AST ElementSection node.
class ElementSection : public Section {
public:
  /// Getter of content vector.
  Span<const ElementSegment> getContent() const { return Content; }

  /// The node type should be ASTNodeAttr::Sec_Element.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Element;

protected:
  /// Overrided content loading of element section.
  Expect<void> loadContent(FileMgr &Mgr,
                           const ProposalConfigure &PConf) override;

private:
  /// Vector of ElementSegment nodes.
  std::vector<ElementSegment> Content;
};

/// AST CodeSection node.
class CodeSection : public Section {
public:
  /// Getter of content vector.
  Span<const CodeSegment> getContent() const { return Content; }

  /// Getter of mutable content vector .
  std::vector<CodeSegment> &getContent() { return Content; }

  /// The node type should be ASTNodeAttr::Sec_Code.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Code;

protected:
  /// Overrided content loading of code section.
  Expect<void> loadContent(FileMgr &Mgr,
                           const ProposalConfigure &PConf) override;

private:
  /// Vector of CodeSegment nodes.
  std::vector<CodeSegment> Content;
};

/// AST DataSection node.
class DataSection : public Section {
public:
  /// Getter of content vector.
  Span<const DataSegment> getContent() const { return Content; }

  /// The node type should be ASTNodeAttr::Sec_Data.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Data;

protected:
  /// Overrided content loading of data section.
  Expect<void> loadContent(FileMgr &Mgr,
                           const ProposalConfigure &PConf) override;

private:
  /// Vector of DataSegment nodes.
  std::vector<DataSegment> Content;
};

/// AST DataCountSection node.
class DataCountSection : public Section {
public:
  /// Getter of content.
  uint32_t getContent() const { return Content; }

  /// The node type should be ASTNodeAttr::Sec_DataCount.
  const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_DataCount;

protected:
  /// Overrided content loading of custom section.
  Expect<void> loadContent(FileMgr &Mgr,
                           const ProposalConfigure &PConf) override;

private:
  /// u32 of count of data segments.
  uint32_t Content;
};

} // namespace AST
} // namespace SSVM
