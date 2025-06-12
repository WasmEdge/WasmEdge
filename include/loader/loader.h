// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

#include "ast/component/component.h"
#include "ast/module.h"
#include "common/configure.h"
#include "common/errinfo.h"
#include "loader/filemgr.h"
#include "loader/serialize.h"
#include "loader/shared_library.h"

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
template <> inline ASTNodeAttr NodeAttrFromAST<AST::TagSection>() noexcept {
  return ASTNodeAttr::Sec_Tag;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::SubType>() noexcept {
  return ASTNodeAttr::Type_Rec;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::FunctionType>() noexcept {
  return ASTNodeAttr::Type_Function;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::ElementSegment>() noexcept {
  return ASTNodeAttr::Seg_Element;
}

template <>
inline ASTNodeAttr
NodeAttrFromAST<AST::Component::CoreModuleSection>() noexcept {
  return ASTNodeAttr::Comp_Sec_CoreMod;
}
template <>
inline ASTNodeAttr
NodeAttrFromAST<AST::Component::CoreInstanceSection>() noexcept {
  return ASTNodeAttr::Comp_Sec_CoreInstance;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::CoreTypeSection>() noexcept {
  return ASTNodeAttr::Comp_Sec_CoreType;
}
template <>
inline ASTNodeAttr
NodeAttrFromAST<AST::Component::ComponentSection>() noexcept {
  return ASTNodeAttr::Comp_Sec_Component;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::InstanceSection>() noexcept {
  return ASTNodeAttr::Comp_Sec_Instance;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::AliasSection>() noexcept {
  return ASTNodeAttr::Comp_Sec_Alias;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::TypeSection>() noexcept {
  return ASTNodeAttr::Comp_Sec_Type;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::CanonSection>() noexcept {
  return ASTNodeAttr::Comp_Sec_Canon;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::StartSection>() noexcept {
  return ASTNodeAttr::Comp_Sec_Start;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::ImportSection>() noexcept {
  return ASTNodeAttr::Comp_Sec_Import;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::ExportSection>() noexcept {
  return ASTNodeAttr::Comp_Sec_Export;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::CoreInstance>() noexcept {
  return ASTNodeAttr::Comp_CoreInstance;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::Instance>() noexcept {
  return ASTNodeAttr::Comp_Instance;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::CoreDefType>() noexcept {
  return ASTNodeAttr::Comp_CoreDefType;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::DefType>() noexcept {
  return ASTNodeAttr::Comp_DefType;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::FuncType>() noexcept {
  return ASTNodeAttr::Comp_FuncType;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::ComponentType>() noexcept {
  return ASTNodeAttr::Comp_ComponentType;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::InstanceType>() noexcept {
  return ASTNodeAttr::Comp_InstanceType;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::ResourceType>() noexcept {
  return ASTNodeAttr::Comp_ResourceType;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::Canonical>() noexcept {
  return ASTNodeAttr::Comp_Canonical;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::Start>() noexcept {
  return ASTNodeAttr::Comp_Start;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::Export>() noexcept {
  return ASTNodeAttr::Comp_Export;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::RecordTy>() noexcept {
  return ASTNodeAttr::Comp_Type_Record;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::VariantTy>() noexcept {
  return ASTNodeAttr::Comp_Type_Variant;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::TupleTy>() noexcept {
  return ASTNodeAttr::Comp_Type_Tuple;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::FlagsTy>() noexcept {
  return ASTNodeAttr::Comp_Type_Flags;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::EnumTy>() noexcept {
  return ASTNodeAttr::Comp_Type_Enum;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::ResultTy>() noexcept {
  return ASTNodeAttr::Comp_Type_Result;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::StreamTy>() noexcept {
  return ASTNodeAttr::Comp_Type_Stream;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::FutureTy>() noexcept {
  return ASTNodeAttr::Comp_Type_Future;
}

} // namespace

/// Loader flow control class.
class Loader {
public:
  Loader(const Configure &Conf,
         const Executable::IntrinsicsTable *IT = nullptr) noexcept
      : Conf(Conf), Ser(Conf), IntrinsicsTable(IT) {}
  ~Loader() noexcept = default;

  /// Load data from file path.
  static Expect<std::vector<Byte>>
  loadFile(const std::filesystem::path &FilePath);

  /// Parse module or component from file path.
  Expect<std::variant<std::unique_ptr<AST::Component::Component>,
                      std::unique_ptr<AST::Module>>>
  parseWasmUnit(const std::filesystem::path &FilePath);

  /// Parse module or component from byte code.
  Expect<std::variant<std::unique_ptr<AST::Component::Component>,
                      std::unique_ptr<AST::Module>>>
  parseWasmUnit(Span<const uint8_t> Code);

  /// Parse module from file path.
  Expect<std::unique_ptr<AST::Module>>
  parseModule(const std::filesystem::path &FilePath);

  /// Parse module from byte code.
  Expect<std::unique_ptr<AST::Module>> parseModule(Span<const uint8_t> Code);

  /// Serialize module into byte code.
  Expect<std::vector<Byte>> serializeModule(const AST::Module &Mod);

  /// Reset status.
  void reset() noexcept { FMgr.reset(); }

  /// Setup Symbol from an Exetuable.
  Expect<void> loadExecutable(AST::Module &Mod,
                              std::shared_ptr<Executable> Library);

private:
  /// \name Helper functions to print error log when loading AST nodes
  /// @{
  inline Unexpected<ErrCode> logLoadError(ErrCode Code, uint64_t Off,
                                          ASTNodeAttr Node) const noexcept {
    spdlog::error(Code);
    spdlog::error(ErrInfo::InfoLoading(Off));
    spdlog::error(ErrInfo::InfoAST(Node));
    return Unexpect(Code);
  }
  inline Unexpected<ErrCode> logNeedProposal(ErrCode Code, Proposal Prop,
                                             uint64_t Off,
                                             ASTNodeAttr Node) const noexcept {
    spdlog::error(Code);
    spdlog::error(ErrInfo::InfoProposal(Prop));
    spdlog::error(ErrInfo::InfoLoading(Off));
    spdlog::error(ErrInfo::InfoAST(Node));
    return Unexpect(Code);
  }
  /// @}

  /// \name Load AST Module functions
  /// @{
  // Load component or module unit.
  Expect<std::variant<std::unique_ptr<AST::Component::Component>,
                      std::unique_ptr<AST::Module>>>
  loadUnit();

  // Load magic header or preamble.
  Expect<std::pair<std::vector<Byte>, std::vector<Byte>>> loadPreamble();

  // Load WASM module.
  Expect<void> loadModule(AST::Module &Mod,
                          std::optional<uint64_t> Bound = std::nullopt);

  // Load WASM for AOT.
  Expect<void> loadUniversalWASM(AST::Module &Mod);
  Expect<void> loadModuleAOT(AST::AOTSection &AOTSection);

  // Load component.
  Expect<void> loadComponent(AST::Component::Component &Comp,
                             std::optional<uint64_t> Bound = std::nullopt);
  /// @}

  /// \name Load AST section node helper functions
  /// @{
  Expect<uint32_t> loadVecCnt() {
    // Read the vector size.
    EXPECTED_TRY(auto Cnt, FMgr.readU32());
    if (Cnt / 2 > FMgr.getRemainSize()) {
      return Unexpect(ErrCode::Value::IntegerTooLong);
    }
    return Cnt;
  }

  template <typename ASTType, typename T, typename ElemLoader>
  Expect<void> loadVec(std::vector<T> &Vec, ElemLoader &&Func) {
    // Read the vector size.
    EXPECTED_TRY(uint32_t VecCnt, loadVecCnt().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(), NodeAttrFromAST<ASTType>());
    }));
    Vec.resize(VecCnt);

    // Sequently create the AST node T and read data.
    for (uint32_t I = 0; I < VecCnt; ++I) {
      EXPECTED_TRY(Func(Vec[I]).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(NodeAttrFromAST<ASTType>()));
        return E;
      }));
    }
    return {};
  }

  template <typename T, typename ElemLoader>
  Expect<void> loadSectionContent(T &Sec, ElemLoader &&Func) {
    Sec.setStartOffset(FMgr.getOffset());
    // Load the section size first.
    EXPECTED_TRY(uint32_t SecSize, FMgr.readU32().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(), NodeAttrFromAST<T>());
    }));

    // Check the remain file/buffer size.
    if (unlikely(FMgr.getRemainSize() < SecSize)) {
      return logLoadError(ErrCode::Value::LengthOutOfBounds,
                          FMgr.getLastOffset(), NodeAttrFromAST<T>());
    }

    // Set the section size.
    Sec.setContentSize(SecSize);
    auto StartOffset = FMgr.getOffset();

    // Invoke the callback function.
    EXPECTED_TRY(Func());

    // Check the read size matches the section size.
    auto EndOffset = FMgr.getOffset();
    if (EndOffset - StartOffset != Sec.getContentSize()) {
      return logLoadError(ErrCode::Value::SectionSizeMismatch, EndOffset,
                          NodeAttrFromAST<T>());
    }
    return {};
  }

  template <typename T, typename ElemLoader>
  Expect<void> loadSectionContentVec(T &Sec, ElemLoader &&Func) {
    return loadVec<T>(Sec.getContent(), std::move(Func));
  }
  /// @}

  /// \name Helper function to set the function type for tag
  /// @{
  void setTagFunctionType(AST::TagSection &TagSec,
                          AST::ImportSection &ImportSec,
                          AST::TypeSection &TypeSec);
  /// @}

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
  Expect<void> loadSection(AST::TagSection &Sec);
  static Expect<void> loadSection(FileMgr &VecMgr, AST::AOTSection &Sec);
  Expect<void> loadSegment(AST::TableSegment &TabSeg);
  Expect<void> loadSegment(AST::GlobalSegment &GlobSeg);
  Expect<void> loadSegment(AST::ElementSegment &ElemSeg);
  Expect<void> loadSegment(AST::CodeSegment &CodeSeg);
  Expect<void> loadSegment(AST::DataSegment &DataSeg);
  Expect<void> loadDesc(AST::ImportDesc &ImpDesc);
  Expect<void> loadDesc(AST::ExportDesc &ExpDesc);
  Expect<ValType> loadHeapType(TypeCode TC, ASTNodeAttr From);
  Expect<ValType> loadRefType(ASTNodeAttr From);
  Expect<ValType> loadValType(ASTNodeAttr From, bool IsStorageType = false);
  Expect<ValMut> loadMutability(ASTNodeAttr From);
  Expect<void> loadFieldType(AST::FieldType &FType);
  Expect<void> loadCompositeType(AST::CompositeType &CType);
  Expect<void> loadLimit(AST::Limit &Lim);
  Expect<void> loadType(AST::SubType &SType);
  Expect<void> loadType(AST::FunctionType &FuncType);
  Expect<void> loadType(AST::MemoryType &MemType);
  Expect<void> loadType(AST::TableType &TabType);
  Expect<void> loadType(AST::GlobalType &GlobType);
  Expect<void> loadType(AST::TagType &TgType);
  Expect<void> loadExpression(AST::Expression &Expr,
                              std::optional<uint64_t> SizeBound = std::nullopt);
  Expect<OpCode> loadOpCode();
  Expect<AST::InstrVec> loadInstrSeq(std::optional<uint64_t> SizeBound);
  Expect<void> loadInstruction(AST::Instruction &Instr);
  /// @}

  /// \name Load AST nodes functions for component model
  /// @{
  // Sections
  Expect<void> loadSection(AST::Component::CoreModuleSection &Sec);
  Expect<void> loadSection(AST::Component::CoreInstanceSection &Sec);
  Expect<void> loadSection(AST::Component::CoreTypeSection &Sec);
  Expect<void> loadSection(AST::Component::ComponentSection &Sec);
  Expect<void> loadSection(AST::Component::InstanceSection &Sec);
  Expect<void> loadSection(AST::Component::AliasSection &Sec);
  Expect<void> loadSection(AST::Component::TypeSection &Sec);
  Expect<void> loadSection(AST::Component::CanonSection &Sec);
  Expect<void> loadSection(AST::Component::StartSection &Sec);
  Expect<void> loadSection(AST::Component::ImportSection &Sec);
  Expect<void> loadSection(AST::Component::ExportSection &Sec);
  // core:instance and instance
  Expect<void> loadCoreInstance(AST::Component::CoreInstance &Instance);
  Expect<void> loadInstance(AST::Component::Instance &Instance);
  // core:sort and sort
  Expect<void> loadCoreSort(AST::Component::Sort &Sort);
  Expect<void> loadSort(AST::Component::Sort &Sort);
  Expect<void> loadSortIndex(AST::Component::SortIndex &SortIdx,
                             const bool IsCore = false);
  // core:alias and alias
  Expect<void> loadCoreAlias(AST::Component::CoreAlias &Alias);
  Expect<void> loadAlias(AST::Component::Alias &Alias);
  // core:deftype and deftype
  Expect<void> loadType(AST::Component::CoreDefType &Ty);
  Expect<void> loadType(AST::Component::DefType &Ty);
  Expect<void> loadType(AST::Component::DefValType &Ty, uint8_t Code);
  Expect<void> loadType(AST::Component::FuncType &Ty);
  Expect<void> loadType(AST::Component::ComponentType &Ty);
  Expect<void> loadType(AST::Component::InstanceType &Ty);
  Expect<void> loadType(AST::Component::ResourceType &Ty);
  // start
  Expect<void> loadStart(AST::Component::Start &S);
  // canonical
  Expect<void> loadCanonical(AST::Component::Canonical &C);
  Expect<void> loadCanonicalOption(AST::Component::CanonOpt &Opt);
  // import
  Expect<void> loadImport(AST::Component::Import &Im);
  // export
  Expect<void> loadExport(AST::Component::Export &Ex);
  // descs
  Expect<void> loadDesc(AST::Component::CoreImportDesc &Desc);
  Expect<void> loadDesc(AST::Component::ExternDesc &Desc);
  // decls
  Expect<void> loadDecl(AST::Component::CoreImportDecl &Decl);
  Expect<void> loadDecl(AST::Component::CoreExportDecl &Decl);
  Expect<void> loadDecl(AST::Component::CoreModuleDecl &Decl);
  Expect<void> loadDecl(AST::Component::ImportDecl &Decl);
  Expect<void> loadDecl(AST::Component::ExportDecl &Decl);
  Expect<void> loadDecl(AST::Component::InstanceDecl &Decl);
  Expect<void> loadDecl(AST::Component::ComponentDecl &Decl);
  // types
  Expect<void> loadType(AST::Component::RecordTy &RecTy);
  Expect<void> loadType(AST::Component::VariantTy &Ty);
  Expect<void> loadType(AST::Component::ListTy &Ty, bool IsFixedLen = false);
  Expect<void> loadType(AST::Component::TupleTy &Ty);
  Expect<void> loadType(AST::Component::FlagsTy &Ty);
  Expect<void> loadType(AST::Component::EnumTy &Ty);
  Expect<void> loadType(AST::Component::OptionTy &Ty);
  Expect<void> loadType(AST::Component::ResultTy &Ty);
  Expect<void> loadType(AST::Component::OwnTy &Ty);
  Expect<void> loadType(AST::Component::BorrowTy &Ty);
  Expect<void> loadType(AST::Component::StreamTy &Ty);
  Expect<void> loadType(AST::Component::FutureTy &Ty);
  // helpers
  Expect<void> loadExternName(std::string &Name);
  Expect<void> loadType(AST::Component::ValueType &Ty);
  Expect<void> loadType(AST::Component::LabelValType &Ty);
  template <typename ASTType, typename T>
  Expect<std::optional<T>> loadOption(std::function<Expect<void>(T &)> F) {
    EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(), NodeAttrFromAST<ASTType>());
    }));
    switch (Flag) {
    case 0x01: {
      T V;
      EXPECTED_TRY(F(V).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(NodeAttrFromAST<ASTType>()));
        return E;
      }));
      return std::optional<T>(V);
    }
    case 0x00:
      return std::nullopt;
    default:
      return logLoadError(ErrCode::Value::MalformedDefType,
                          FMgr.getLastOffset(), NodeAttrFromAST<ASTType>());
    }
  }
  /// @}

  /// \name Loader members
  /// @{
  const Configure Conf;
  const Serializer Ser;
  FileMgr FMgr;
  const Executable::IntrinsicsTable *IntrinsicsTable;
  std::recursive_mutex Mutex;
  bool HasDataSection;

  /// Input data type enumeration.
  enum class InputType : uint8_t { WASM, UniversalWASM, SharedLibrary };
  InputType WASMType = InputType::WASM;
  /// @}

  // Metadata
  std::vector<Byte> ModuleVersion = {0x01, 0x00, 0x00, 0x00};
  // spec says 0x0a, but it's actually 0x0d, where cargo component compiled out
  std::vector<Byte> ComponentVersion = {0x0d, 0x00, 0x01, 0x00};
};

} // namespace Loader
} // namespace WasmEdge
