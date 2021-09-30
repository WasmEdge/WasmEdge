// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/ast/section.h - Section class definition -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Section node class, which is the
/// module node in AST, and the subsection classes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/base.h"
#include "ast/description.h"
#include "ast/segment.h"
#include "ast/type.h"

#include <optional>
#include <vector>

namespace WasmEdge {
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
  /// \param Conf the WasmEdge configuration reference.
  ///
  /// \returns void when success, ErrCode when failed.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

protected:
  /// Read content size of this section.
  Expect<void> loadSize(FileMgr &Mgr);

  /// Read content of this section.
  virtual Expect<void> loadContent(FileMgr &Mgr, const Configure &Conf) = 0;

  /// Template function of reading vector of type T.
  ///
  /// Helper function of read variaties of vectors.
  /// Read the content size of this section.
  /// Call loadContent() for reading contents.
  ///
  /// \param Mgr the file manager reference.
  /// \param Conf the proposal configuration reference.
  /// \param Node the node type of caller.
  /// \param [out] Vec filled with read data on loadVector success.
  ///
  /// \returns void when success, ErrCode when failed.
  template <typename T>
  Expect<void> loadToVector(FileMgr &Mgr, const Configure &Conf,
                            const ASTNodeAttr Node, std::vector<T> &Vec) {
    auto StartOffset = Mgr.getOffset();
    uint32_t VecCnt = 0;
    /// Read vector size.
    if (auto Res = Mgr.readU32()) {
      VecCnt = *Res;
      /// A section may be splited into partitions in module.
      Vec.reserve(Vec.size() + VecCnt);
    } else {
      return logLoadError(Res.error(), Mgr.getLastOffset(), Node);
    }

    /// Sequently create AST node T and read data.
    for (uint32_t i = 0; i < VecCnt; ++i) {
      Vec.emplace_back();
      if (auto Res = Vec.back().loadBinary(Mgr, Conf); !Res) {
        spdlog::error(ErrInfo::InfoAST(Node));
        return Unexpect(Res);
      }
    }
    /// Check the read size match the section size.
    auto EndOffset = Mgr.getOffset();
    if (EndOffset - StartOffset != ContentSize) {
      return logLoadError(ErrCode::SectionSizeMismatch, EndOffset, Node);
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
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Custom;

  /// Getter of name.
  const std::string &getName() const { return Name; }

  /// Getter of content vector.
  const std::vector<Byte> &getContent() const { return Content; }

protected:
  /// Overrided content loading of custom section.
  Expect<void> loadContent(FileMgr &Mgr, const Configure &Conf) override;

private:
  /// Name of this custom secion.
  std::string Name;
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
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Type;

protected:
  /// Overrided content loading of type section.
  Expect<void> loadContent(FileMgr &Mgr, const Configure &Conf) override;

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
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Import;

protected:
  /// Overrided content loading of import section.
  Expect<void> loadContent(FileMgr &Mgr, const Configure &Conf) override;

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
  static inline constexpr const ASTNodeAttr NodeAttr =
      ASTNodeAttr::Sec_Function;

protected:
  /// Overrided content loading of function section.
  Expect<void> loadContent(FileMgr &Mgr, const Configure &Conf) override;

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
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Table;

protected:
  /// Overrided content loading of table section.
  Expect<void> loadContent(FileMgr &Mgr, const Configure &Conf) override;

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
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Memory;

protected:
  /// Overrided content loading of memory section.
  Expect<void> loadContent(FileMgr &Mgr, const Configure &Conf) override;

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
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Global;

protected:
  /// Overrided content loading of global section.
  Expect<void> loadContent(FileMgr &Mgr, const Configure &Conf) override;

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
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Export;

protected:
  /// Overrided content loading of export section.
  Expect<void> loadContent(FileMgr &Mgr, const Configure &Conf) override;

private:
  /// Vector of ExportDesc nodes.
  std::vector<ExportDesc> Content;
};

/// AST StartSection node.
class StartSection : public Section {
public:
  /// Getter of content.
  std::optional<uint32_t> getContent() const { return Content; }

  /// The node type should be ASTNodeAttr::Sec_Start.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Start;

protected:
  /// Overrided content loading of start section.
  Expect<void> loadContent(FileMgr &Mgr, const Configure &Conf) override;

private:
  /// Start function index.
  std::optional<uint32_t> Content = std::nullopt;
};

/// AST ElementSection node.
class ElementSection : public Section {
public:
  /// Getter of content vector.
  Span<const ElementSegment> getContent() const { return Content; }

  /// The node type should be ASTNodeAttr::Sec_Element.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Element;

protected:
  /// Overrided content loading of element section.
  Expect<void> loadContent(FileMgr &Mgr, const Configure &Conf) override;

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
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Code;

protected:
  /// Overrided content loading of code section.
  Expect<void> loadContent(FileMgr &Mgr, const Configure &Conf) override;

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
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_Data;

protected:
  /// Overrided content loading of data section.
  Expect<void> loadContent(FileMgr &Mgr, const Configure &Conf) override;

private:
  /// Vector of DataSegment nodes.
  std::vector<DataSegment> Content;
};

/// AST DataCountSection node.
class DataCountSection : public Section {
public:
  /// Getter of content.
  std::optional<uint32_t> getContent() const { return Content; }

  /// The node type should be ASTNodeAttr::Sec_DataCount.
  static inline constexpr const ASTNodeAttr NodeAttr =
      ASTNodeAttr::Sec_DataCount;

protected:
  /// Overrided content loading of custom section.
  Expect<void> loadContent(FileMgr &Mgr, const Configure &Conf) override;

private:
  /// u32 of count of data segments.
  std::optional<uint32_t> Content = std::nullopt;
};

class AOTSection : public Base {
public:
  /// Binary loading from file manager.
  Expect<void> loadBinary(FileMgr &Mgr, const Configure &Conf) override;

  /// AST node attribute.
  static inline constexpr const ASTNodeAttr NodeAttr = ASTNodeAttr::Sec_AOT;

  constexpr auto getVersionAddress() const noexcept { return VersionAddress; }
  constexpr auto getIntrinsicsAddress() const noexcept {
    return IntrinsicsAddress;
  }
  constexpr const auto &getTypesAddress() const noexcept {
    return TypesAddress;
  }
  constexpr const auto &getCodesAddress() const noexcept {
    return CodesAddress;
  }
  constexpr const auto &getSections() const noexcept { return Sections; }

private:
  uint32_t Version;
  uint8_t OSType;
  uint8_t ArchType;
  uint64_t VersionAddress;
  uint64_t IntrinsicsAddress;
  std::vector<uintptr_t> TypesAddress;
  std::vector<uintptr_t> CodesAddress;
  std::vector<std::tuple<uint8_t, uint64_t, uint64_t, std::vector<Byte>>>
      Sections;
};

} // namespace AST
} // namespace WasmEdge
