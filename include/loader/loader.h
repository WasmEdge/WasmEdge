// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/loader/loader.h - Loader flow control class definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Loader class, which controls flow
/// of WASM loading.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common/configure.h"
#include "common/errinfo.h"
#include "common/log.h"
#include "loader/filemgr.h"
#include "loader/ldmgr.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace WasmEdge {
namespace Loader {

namespace {
template <typename T> inline ASTNodeAttr NodeAttrFromAST() noexcept;
template <> inline ASTNodeAttr NodeAttrFromAST<AST::CustomSection>() noexcept {
  return ASTNodeAttr::Sec_Custom;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::TypeSection>() noexcept {
  return ASTNodeAttr::Sec_Type;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::ImportSection>() noexcept {
  return ASTNodeAttr::Sec_Import;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::FunctionSection>() noexcept {
  return ASTNodeAttr::Sec_Function;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::TableSection>() noexcept {
  return ASTNodeAttr::Sec_Table;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::MemorySection>() noexcept {
  return ASTNodeAttr::Sec_Memory;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::GlobalSection>() noexcept {
  return ASTNodeAttr::Sec_Global;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::ExportSection>() noexcept {
  return ASTNodeAttr::Sec_Export;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::StartSection>() noexcept {
  return ASTNodeAttr::Sec_Start;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::ElementSection>() noexcept {
  return ASTNodeAttr::Sec_Element;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::CodeSection>() noexcept {
  return ASTNodeAttr::Sec_Code;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::DataSection>() noexcept {
  return ASTNodeAttr::Sec_Data;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::DataCountSection>() noexcept {
  return ASTNodeAttr::Sec_DataCount;
}
} // namespace

/// Loader flow control class.
class Loader {
public:
  Loader(const Configure &Conf,
         const AST::Module::IntrinsicsTable *IT = nullptr) noexcept
      : Conf(Conf), LMgr(IT), IntrinsicsTable(IT) {}
  ~Loader() noexcept = default;

  /// Load data from file path.
  static Expect<std::vector<Byte>>
  loadFile(const std::filesystem::path &FilePath);

  /// Parse module from file path.
  Expect<std::unique_ptr<AST::Module>>
  parseModule(const std::filesystem::path &FilePath);

  /// Parse module from byte code.
  Expect<std::unique_ptr<AST::Module>> parseModule(Span<const uint8_t> Code);

private:
  /// \name Helper functions to print error log when loading AST nodes
  /// @{
  inline auto logLoadError(ErrCode Code, uint64_t Off,
                           ASTNodeAttr Node) const noexcept {
    spdlog::error(Code);
    spdlog::error(ErrInfo::InfoLoading(Off));
    spdlog::error(ErrInfo::InfoAST(Node));
    return Unexpect(Code);
  }
  inline auto logNeedProposal(ErrCode Code, Proposal Prop, uint64_t Off,
                              ASTNodeAttr Node) const noexcept {
    spdlog::error(Code);
    spdlog::error(ErrInfo::InfoProposal(Prop));
    spdlog::error(ErrInfo::InfoLoading(Off));
    spdlog::error(ErrInfo::InfoAST(Node));
    return Unexpect(Code);
  }
  Expect<ValType> checkValTypeProposals(ValType VType, uint64_t Off,
                                        ASTNodeAttr Node) const noexcept;
  Expect<RefType> checkRefTypeProposals(RefType RType, uint64_t Off,
                                        ASTNodeAttr Node) const noexcept;
  Expect<void> checkInstrProposals(OpCode Code, uint64_t Offset) const noexcept;
  /// @}

  /// \name Load AST Module functions
  /// @{
  Expect<std::unique_ptr<AST::Module>> loadModule();
  Expect<void> loadCompiled(AST::Module &Mod);
  /// @}

  /// \name Load AST section node helper functions
  /// @{
  Expect<uint32_t> loadSectionSize(ASTNodeAttr Node);
  template <typename T, typename L>
  Expect<void> loadSectionContent(T &Sec, L &&Func) {
    Sec.setStartOffset(FMgr.getOffset());
    if (auto Res = loadSectionSize(NodeAttrFromAST<T>())) {
      // Set the section size.
      Sec.setContentSize(*Res);
      auto StartOffset = FMgr.getOffset();
      auto ResContent = Func();
      if (!ResContent) {
        return Unexpect(ResContent);
      }
      // Check the read size match the section size.
      auto EndOffset = FMgr.getOffset();
      if (EndOffset - StartOffset != Sec.getContentSize()) {
        return logLoadError(ErrCode::Value::SectionSizeMismatch, EndOffset,
                            NodeAttrFromAST<T>());
      }
    } else {
      return Unexpect(Res);
    }
    return {};
  }
  template <typename T, typename L>
  Expect<void> loadSectionContentVec(T &Sec, L &&Func) {
    uint32_t VecCnt = 0;
    // Read the vector size.
    if (auto Res = FMgr.readU32()) {
      VecCnt = *Res;
      if (VecCnt / 2 > FMgr.getRemainSize()) {
        return logLoadError(ErrCode::Value::IntegerTooLong,
                            FMgr.getLastOffset(), NodeAttrFromAST<T>());
      }
      Sec.getContent().resize(VecCnt);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          NodeAttrFromAST<T>());
    }

    // Sequently create the AST node T and read data.
    for (uint32_t I = 0; I < VecCnt; ++I) {
      if (auto Res = Func(Sec.getContent()[I]); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttrFromAST<T>()));
        return Unexpect(Res);
      }
    }
    return {};
  }

  /// \name Load AST nodes functions
  /// @{
  Expect<void> loadSection(AST::CustomSection &Sec);
  Expect<void> loadSection(AST::TypeSection &Sec);
  Expect<void> loadSection(AST::ImportSection &Sec);
  Expect<void> loadSection(AST::FunctionSection &Sec);
  Expect<void> loadSection(AST::TableSection &Sec);
  Expect<void> loadSection(AST::MemorySection &Sec);
  Expect<void> loadSection(AST::GlobalSection &Sec);
  Expect<void> loadSection(AST::ExportSection &Sec);
  Expect<void> loadSection(AST::StartSection &Sec);
  Expect<void> loadSection(AST::ElementSection &Sec);
  Expect<void> loadSection(AST::CodeSection &Sec);
  Expect<void> loadSection(AST::DataSection &Sec);
  Expect<void> loadSection(AST::DataCountSection &Sec);
  static Expect<void> loadSection(FileMgr &VecMgr, AST::AOTSection &Sec);
  Expect<void> loadSegment(AST::GlobalSegment &GlobSeg);
  Expect<void> loadSegment(AST::ElementSegment &ElemSeg);
  Expect<void> loadSegment(AST::CodeSegment &CodeSeg);
  Expect<void> loadSegment(AST::DataSegment &DataSeg);
  Expect<void> loadDesc(AST::ImportDesc &ImpDesc);
  Expect<void> loadDesc(AST::ExportDesc &ExpDesc);
  Expect<void> loadLimit(AST::Limit &Lim);
  Expect<void> loadType(AST::FunctionType &FuncType);
  Expect<void> loadType(AST::MemoryType &MemType);
  Expect<void> loadType(AST::TableType &TabType);
  Expect<void> loadType(AST::GlobalType &GlobType);
  Expect<void> loadExpression(AST::Expression &Expr,
                              std::optional<uint64_t> SizeBound = std::nullopt);
  Expect<OpCode> loadOpCode();
  Expect<AST::InstrVec> loadInstrSeq(std::optional<uint64_t> SizeBound);
  Expect<void> loadInstruction(AST::Instruction &Instr);
  /// @}

  /// \name Loader members
  /// @{
  const Configure Conf;
  FileMgr FMgr;
  LDMgr LMgr;
  const AST::Module::IntrinsicsTable *IntrinsicsTable;
  std::recursive_mutex Mutex;
  bool HasDataSection;

  /// Input data type enumeration.
  enum class InputType : uint8_t { WASM, UniversalWASM, SharedLibrary };
  InputType WASMType = InputType::WASM;
  /// @}
};

} // namespace Loader
} // namespace WasmEdge
