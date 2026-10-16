// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "tree_sitter.h"

#include "ast/module.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/hash.h"
#include "common/span.h"
#include "common/types.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace WAT {

/// Append the End instruction that closes the expression of Instrs.
void appendExprEnd(AST::InstrVec &Instrs);

class Converter {
public:
  explicit Converter(const Configure &C) : Conf(C) {}

  /// Convert a parsed WAT tree into an AST::Module.
  Expect<AST::Module> convert(const Tree &ParseTree, std::string_view Source);

private:
  // --- The checks of the source before the conversion ---
  Expect<void>
  scanSourceCharacters(std::vector<size_t> &AnnotationStarts) const;
  Expect<void> checkReservedLeaf(bool HasError, Node Leaf) const;
  Expect<void> checkAnnotationId(Node Ann) const;
  Expect<void> checkBareAnnotations(Node Root,
                                    Span<const size_t> AnnotationStarts) const;
  Expect<void> checkAdjacentTokens(Node Prev, Node Curr) const;

  // --- Symbol Table ---
  struct SymbolTable {
    enum class IndexSpace : uint8_t {
      Type,
      Func,
      Table,
      Memory,
      Global,
      Tag,
      Elem,
      Data,
      Local,
    };
    static constexpr size_t SpaceCount =
        static_cast<size_t>(IndexSpace::Local) + 1;
    using NameMap = std::unordered_map<std::string, uint32_t, Hash::Hash>;

    // The names and the next index of each index space.
    std::array<NameMap, SpaceCount> Names;
    std::array<uint32_t, SpaceCount> Next{};
    std::unordered_map<std::string, std::vector<uint32_t>, Hash::Hash> Labels;
    // The map from a struct field name to a field index, for each type.
    std::unordered_map<uint32_t, NameMap> FieldNames;
    std::vector<std::string> LabelStack;

    void clear();
    void clearLocals();
    Expect<void> pushLabel(std::string_view Label);
    void popLabel();

    uint32_t &next(IndexSpace Space) {
      return Next[static_cast<size_t>(Space)];
    }
    /// Give the next index of the space Space to the name Name, and advance
    /// the index. An empty name only advances the index. If the name exists,
    /// return the error code DupErr.
    Expect<void> define(IndexSpace Space, std::string Name,
                        ErrCode::Value DupErr);

    Expect<uint32_t> resolveIdx(const NameMap &Map, std::string_view Ref,
                                ErrCode::Value Err) const;
    Expect<uint32_t> resolve(IndexSpace Space, std::string_view Ref) const;
    Expect<uint32_t> resolveLabel(std::string_view Ref) const;
    Expect<uint32_t> resolveField(uint32_t TypeIdx, std::string_view Ref) const;
  };

  /// The kind of a func, table, memory, global, or tag field, import, or
  /// export.
  struct ExternKind {
    ExternalType Type;
    SymbolTable::IndexSpace Space;
    /// The error code of a name that the space already holds.
    ErrCode::Value DupErr;
  };
  /// Give the kind of the keyword KW, or no value if KW is not such a kind.
  static std::optional<ExternKind> externKind(std::string_view KW);

  // --- The module fields ---
  /// Give the module fields of the root. The root holds one (module ...)
  /// sexpr, or a bare sequence of module fields. Anything else at the root
  /// is malformed.
  Expect<std::vector<Node>> moduleFields(Node Root) const;

  // --- Pass 1: Collect identifiers and assign indices ---
  Expect<void> collectIndices(Span<const Node> Fields);
  Expect<void> collectModuleField(Node Field);

  // --- Pass 2: Build AST sections ---
  Expect<void> buildModule(Node Root, Span<const Node> Fields,
                           AST::Module &Mod);
  Expect<void> buildModuleField(Node Field, AST::Module &Mod);
  void collectImplicitTypes(Node Root, AST::Module &Mod);
  void finalizeModule(AST::Module &Mod);

  // Type conversions
  Expect<void> convertTypedef(Node N, AST::Module &Mod);
  Expect<void> convertRecType(Node N, AST::Module &Mod);
  Expect<AST::SubType> convertSubType(Node N, uint32_t TypeIdx);
  Expect<void> convertCompType(Node N, std::string_view KW, uint32_t TypeIdx,
                               AST::CompositeType &CompType);
  Expect<AST::FunctionType> convertFuncType(Node N);
  Expect<ValType> convertValType(Node N);
  Expect<ValType> convertRefType(Node N);
  Expect<ValType> convertRefTypeSexpr(Node N);
  Expect<ValType> convertHeapType(Node N);
  Expect<std::vector<AST::FieldType>> convertStructFields(Node N,
                                                          uint32_t TypeIdx);
  Expect<AST::FieldType> convertArrayField(Node N);
  Expect<AST::FieldType> convertFieldType(Node N);
  Expect<ValType> convertStorageType(Node N);
  Expect<ValType>
  convertMutOperand(Node MutNode, Expect<ValType> (Converter::*Convert)(Node));
  Expect<AST::FieldType> parseMutField(Node MutNode);

  // Section conversions
  Expect<void> takeImportNames(Cursor &C, AST::ImportDesc &Desc);
  Expect<std::pair<ExternKind, Cursor>> takeExternDesc(Cursor &C);
  Expect<AST::ImportDesc> convertInlineImport(Cursor &FC);
  Expect<void> convertInlineExport(Cursor &FC, ExternalType ExtType,
                                   uint32_t Idx, AST::Module &Mod);
  Expect<void> convertImport(Node N, AST::Module &Mod);
  void addImport(AST::ImportDesc Desc, AST::Module &Mod);
  Expect<void> convertImportDesc(ExternalType ExtType, Cursor &C,
                                 AST::ImportDesc &Desc, AST::Module &Mod);
  Expect<bool> convertInlineImportExport(Cursor &C, ExternalType ExtType,
                                         uint32_t Idx, AST::Module &Mod);
  Expect<void> convertExport(Node N, AST::Module &Mod);
  void addExport(std::string_view Name, ExternalType ExtType, uint32_t Idx,
                 AST::Module &Mod);
  Expect<void> convertFunc(Node N, AST::Module &Mod);
  Expect<std::optional<uint32_t>> beginImportableField(Cursor &C,
                                                       ExternalType ExtType,
                                                       size_t DefCount,
                                                       AST::Module &Mod);
  Expect<void> convertTable(Node N, AST::Module &Mod);
  Expect<void> convertTableInlineElem(Cursor &C, uint32_t TableIdx, bool Is64,
                                      AST::Module &Mod);
  Expect<void> convertMemory(Node N, AST::Module &Mod);
  Expect<void> convertGlobal(Node N, AST::Module &Mod);
  Expect<void> convertStart(Node N, AST::Module &Mod);
  Expect<void> convertElem(Node N, AST::Module &Mod);
  Expect<void> convertData(Node N, AST::Module &Mod);
  Expect<void> convertTag(Node N, AST::Module &Mod);
  Expect<void> convertWrappedExpr(Node N, std::string_view KW,
                                  AST::Expression &Expr);
  Expect<void> convertOffset(Cursor &C, AST::Expression &Expr);
  Expect<AST::Expression> convertElemItem(Node N);
  Expect<void> appendElemItems(Cursor &C, std::optional<bool> IsExpr,
                               std::vector<AST::Expression> &Out);
  bool hasSegmentIds() const;
  Expect<void> needSegmentIds() const;
  Expect<bool> takeLegacySegmentTarget(Cursor &C, SymbolTable::IndexSpace Space,
                                       uint32_t &Idx);
  Expect<bool> takeSegmentUse(Cursor &C, std::string_view KW,
                              SymbolTable::IndexSpace Space, uint32_t &Idx);
  Expect<void> appendDataString(Cursor &C, std::vector<Byte> &Out);

  // Instruction conversions
  Expect<void> convertExpression(Cursor &C, AST::Expression &Expr);
  Expect<void> convertMixedInstrSeq(Cursor &C, AST::InstrVec &Instrs);

  Expect<AST::Instruction> convertPlainInstr(Cursor &C, OpCode Code,
                                             uint32_t Offset);
  Expect<AST::Instruction> dispatchPlainInstr(Cursor &C, OpCode Code,
                                              uint32_t Offset);
  Expect<void> convertFoldedInstr(Node N, AST::InstrVec &Instrs);

  // The explicit stack of the instruction converter.
  struct InstrFrame;
  Expect<void> runInstrFrames(std::vector<InstrFrame> &Stack,
                              AST::InstrVec &Instrs);
  Expect<void> convertFlatInstr(InstrFrame &F, Node Child,
                                AST::InstrVec &Instrs);
  Expect<void> convertFoldedBlock(std::vector<InstrFrame> &Stack,
                                  AST::InstrVec &Instrs);
  Expect<void> checkClosingLabel(Cursor &C);
  Expect<void> parseCatchClauses(Cursor &C,
                                 AST::Instruction::TryDescriptor &TryDesc);
  static void closeBlock(AST::InstrVec &Instrs, uint32_t BlockIdx);

  // The category functions. The flat path (convertMixedInstrSeq) and the
  // folded path (convertPlainInstr) both use them. The cursor points at the
  // first immediate after the opcode keyword.
  Expect<AST::Instruction> convertSelectOp(Cursor &C, uint32_t Offset);
  Expect<AST::Instruction> convertConstOp(Cursor &C, OpCode Code,
                                          uint32_t Offset);
  Expect<AST::Instruction> convertVarOp(Cursor &C, OpCode Code,
                                        uint32_t Offset);
  Expect<AST::Instruction> convertBranchOp(Cursor &C, OpCode Code,
                                           uint32_t Offset);
  Expect<AST::Instruction> convertCallOp(Cursor &C, OpCode Code,
                                         uint32_t Offset);
  Expect<AST::Instruction> convertRefOp(Cursor &C, OpCode Code,
                                        uint32_t Offset);
  Expect<AST::Instruction> convertTableOp(Cursor &C, OpCode Code,
                                          uint32_t Offset);
  Expect<AST::Instruction> convertMemControlOp(Cursor &C, OpCode Code,
                                               uint32_t Offset);
  Expect<AST::Instruction> convertGCOp(Cursor &C, OpCode Code, uint32_t Offset);
  Expect<AST::Instruction> convertSimdConstOp(Cursor &C, OpCode Code,
                                              uint32_t Offset);
  Expect<AST::Instruction> convertMemLoadStoreOp(Cursor &C, OpCode Code,
                                                 uint32_t Offset);
  Expect<AST::Instruction> convertSimdLaneOp(Cursor &C, OpCode Code,
                                             uint32_t Offset);

  // ERROR node classification
  ErrCode::Value classifyError(Node Root) const;

  // Helpers
  std::string_view nodeText(Node N) const;
  /// Return true if N is an Id or a U token.
  static bool isIndexOrId(Node N);
  /// Make sure that N is an Id or a U token.
  static Expect<void> expectIndexOrId(Node N);

  /// Give the node type at the current position of the cursor C.
  NodeType peekType(const Cursor &C) const;
  /// Give the text of the keyword at C, or an empty view if C is not at a
  /// keyword.
  std::string_view peekKeyword(const Cursor &C) const;

  /// Make sure that C is at the end of its sexpr.
  static Expect<void> expectEnd(const Cursor &C) {
    if (unlikely(C.valid())) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    return {};
  }

  /// Return true if C is at a Sexpr, and its first keyword child is KW.
  bool sexprMatch(const Cursor &C, std::string_view KW) const;
  /// Return true if N is a Sexpr, and its first keyword child is KW.
  bool sexprMatch(Node N, std::string_view KW) const;

  /// Take the index of the space Space at C, and move C past it. The token
  /// must be an Id or a U.
  Expect<uint32_t> takeIdx(Cursor &C, SymbolTable::IndexSpace Space);
  /// Take a type index at C, as takeIdx does.
  Expect<uint32_t> takeTypeIdx(Cursor &C) {
    return takeIdx(C, SymbolTable::IndexSpace::Type);
  }
  /// Take zero or two indices of the space Space at C, as table.copy and
  /// memory.copy do. An absent index is 0.
  Expect<std::pair<uint32_t, uint32_t>>
  takeOptIdxPair(Cursor &C, SymbolTable::IndexSpace Space);
  /// Take an optional index of the space First and an index of the space
  /// Second at C, as table.init and memory.init do. An absent index is 0.
  Expect<std::pair<uint32_t, uint32_t>>
  takeOptIdxThenIdx(Cursor &C, SymbolTable::IndexSpace First,
                    SymbolTable::IndexSpace Second);
  /// Take a label index at C, as takeIdx does.
  Expect<uint32_t> takeLabelIdx(Cursor &C);
  /// If C is at an Id or a U, take the index of the space Space into Idx.
  /// Otherwise, keep Idx and C.
  Expect<void> takeOptIdx(Cursor &C, SymbolTable::IndexSpace Space,
                          uint32_t &Idx);
  /// If C is at an Id, give its text and move C past it. Otherwise, give an
  /// empty view.
  std::string_view takeOptionalId(Cursor &C) const;
  /// Take the string token at C as a name, and move C past it.
  Expect<std::string> takeName(Cursor &C);

  /// The clauses of a typeuse or of a block type.
  struct TypeUseClauses {
    std::optional<uint32_t> TypeIdx;
    AST::FunctionType Inline;
    /// The id of each inline parameter, or an empty string.
    std::vector<std::string> ParamNames;
    bool HasParam = false;
    bool HasResult = false;
    bool hasInline() const noexcept { return HasParam || HasResult; }
  };
  Expect<TypeUseClauses> parseTypeUseClauses(Cursor &C, bool AcceptParamId);
  Expect<uint32_t> resolveTypeUse(TypeUseClauses &Use, AST::Module &Mod);
  Expect<uint32_t> resolveTypeUse(Cursor &C, AST::Module &Mod,
                                  bool AcceptParamId = true);
  Expect<void> checkTypeUseIdx(const AST::Module &Mod, uint32_t TypeIdx,
                               bool HasInline,
                               const AST::FunctionType &Inline) const;
  Expect<void> parseParamList(Cursor &FC, std::vector<ValType> &Types,
                              bool AcceptParamId,
                              std::vector<std::string> *Names,
                              ErrCode::Value MissingTypeErr);
  Expect<void> parseResultList(Cursor &RC, std::vector<ValType> &Types);
  std::optional<bool> peekAddrType(const Cursor &C) const;
  bool atLimits(const Cursor &C, std::optional<bool> AddrType) const;
  Expect<void> takeAbbrevAddrType(Cursor &C, bool Is64);
  Expect<AST::Limit> parseLimits(Cursor &C, bool AllowShared = true);
  Expect<AST::TableType> parseTabletype(Cursor &C);
  Expect<AST::GlobalType> parseGlobaltype(Cursor &C);
  Expect<BlockType> parseBlockType(Cursor &C);

  /// Find a function type that is equal to FuncTy. If there is no such type,
  /// make a new one. The function returns the type index.
  uint32_t findOrCreateFuncType(AST::FunctionType FuncTy, AST::Module &Mod);

  /// Make sure that the proposal P is on. If it is off, log the proposal and
  /// return the error code Code. The code is the same code that the binary
  /// loader gives for the same module, so that the two front ends agree.
  Expect<void> needProposal(Proposal P, ErrCode::Value Code) const;
  /// Make sure that the proposals of the instruction Code are on.
  Expect<void> checkInstrProposal(OpCode Code) const;
  /// Make sure that a memory index that is not zero has the Multiple Memories
  /// proposal. memory.size, memory.grow, memory.fill, memory.copy, and
  /// memory.init use this check. A load or a store holds its memory index in
  /// the memarg flags, and it uses a different error code.
  Expect<void> checkMemIdxProposal(uint32_t MemIdx) const;
  /// Make sure that a mutable global import has its proposal.
  Expect<void> checkMutGlobalImport(const AST::GlobalType &GT) const;
  /// Return true if the field N holds an inline (import ...) sexpr.
  bool hasInlineImport(Node N) const;
  /// Return true if the field N holds a child sexpr that starts with KW.
  bool hasInlineSexpr(Node N, std::string_view KW) const;
  /// Give the error code of a reference type that a proposal gates. The binary
  /// loader gives a different code before the Reference Types proposal.
  ErrCode::Value refTypeFailCode() const;

  // State
  SymbolTable Syms;
  std::string_view Source;
  // The module that buildModule builds. It is null outside of buildModule.
  AST::Module *CurMod = nullptr;
  const Configure &Conf;
  // The implicit-type walk resolves a typeuse before pass 2 and ignores its
  // errors. While this flag is true, needProposal does not log, so that the
  // log holds only the errors that pass 2 reports.
  bool QuietProposal = false;

  // The state of the import order. convert() resets it for each module.
  struct OrderState {
    bool HasFuncDef = false;
    bool HasTableDef = false;
    bool HasMemoryDef = false;
    bool HasGlobalDef = false;
    bool HasTagDef = false;
    bool HasStart = false;
  } Order;
  // The count of the imports of each external type. convert() resets it for
  // each module.
  static constexpr size_t ExternCount =
      static_cast<size_t>(ExternalType::Tag) + 1;
  std::array<uint32_t, ExternCount> ImportCounts{};
  // The first index of each function type that an inline typeuse can use
  // again. The key comes from funcTypeKey. The types before the index
  // FuncTypesSeen are in the map. convert() resets both for each module.
  std::unordered_map<std::string, uint32_t, Hash::Hash> FuncTypeIndices;
  size_t FuncTypesSeen = 0;
};

} // namespace WAT
} // namespace WasmEdge
