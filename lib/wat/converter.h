// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "tree_sitter.h"

#include "ast/module.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/hash.h"
#include "common/types.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace WasmEdge {
namespace WAT {

/// The node types of the simplified grammar. The grammar gives only the
/// S-expression structure and the raw token types. The converter gets the
/// semantics when it matches the keywords.
enum class NodeType {
  Sexpr, // The tree structure. Root and sexpr both map here.

  // Leaf token types
  Keyword,
  U,
  S,
  F,
  String,
  Id,
  Reserved,

  Error,   // A built-in type of tree-sitter.
  Unknown, // The fallback for a type that the converter does not know.
};

class Converter {
public:
  explicit Converter(const Configure &C) : Conf(C) {}

  /// Convert a parsed WAT tree into an AST::Module.
  Expect<AST::Module> convert(const Tree &ParseTree, std::string_view Source);

private:
  // --- Symbol Table ---
  struct SymbolTable {
    std::unordered_map<std::string, uint32_t, Hash::Hash> Types;
    std::unordered_map<std::string, uint32_t, Hash::Hash> Funcs;
    std::unordered_map<std::string, uint32_t, Hash::Hash> Tables;
    std::unordered_map<std::string, uint32_t, Hash::Hash> Memories;
    std::unordered_map<std::string, uint32_t, Hash::Hash> Globals;
    std::unordered_map<std::string, uint32_t, Hash::Hash> Tags;
    std::unordered_map<std::string, uint32_t, Hash::Hash> Elems;
    std::unordered_map<std::string, uint32_t, Hash::Hash> Datas;
    std::unordered_map<std::string, uint32_t, Hash::Hash> Locals;
    std::unordered_map<std::string, std::vector<uint32_t>, Hash::Hash> Labels;
    // The map from a struct field name to a field index, for each type.
    std::unordered_map<uint32_t,
                       std::unordered_map<std::string, uint32_t, Hash::Hash>>
        FieldNames;
    std::vector<std::string> LabelStack;

    uint32_t NextType = 0, NextFunc = 0, NextTable = 0, NextMemory = 0;
    uint32_t NextGlobal = 0, NextTag = 0, NextElem = 0, NextData = 0;
    uint32_t NextLocal = 0;

    void clear();
    void clearLocals();
    void pushLabel(std::string_view Label);
    void popLabel();

    static Expect<void> isIndexOrId(Node N);

    Expect<uint32_t>
    resolveIdx(const std::unordered_map<std::string, uint32_t, Hash::Hash> &Map,
               std::string_view Ref, ErrCode::Value Err) const;

    enum class IndexSpace {
      Func,
      Table,
      Memory,
      Global,
      Tag,
      Elem,
      Data,
      Local,
    };

    Expect<uint32_t> resolve(IndexSpace Space, std::string_view Ref) const;
    Expect<uint32_t> resolveType(std::string_view Ref) const;
    Expect<uint32_t> resolveLabel(std::string_view Ref) const;
    Expect<uint32_t> resolveField(uint32_t TypeIdx, std::string_view Ref) const;
  };

  // --- Pass 1: Collect identifiers and assign indices ---
  Expect<void> collectIndices(Node Root);
  Expect<void> collectModuleField(Node Field);

  // --- Pass 2: Build AST sections ---
  Expect<void> buildModule(Node Root, AST::Module &Mod);
  Expect<void> buildModuleField(Node Field, AST::Module &Mod);

  // Type conversions
  Expect<void> convertTypedef(Node N, AST::Module &Mod);
  Expect<void> convertRecType(Node N, AST::Module &Mod);
  Expect<AST::SubType> convertSubType(Node N, uint32_t TypeIdx);
  Expect<AST::FunctionType> convertFuncType(Node N);
  Expect<ValType> convertValType(Node N);
  Expect<ValType> convertRefType(Node N);
  Expect<ValType> convertRefTypeSexpr(Node N);
  Expect<ValType> convertHeapType(Node N);
  Expect<std::vector<AST::FieldType>> convertStructFields(Node N,
                                                          uint32_t TypeIdx);
  Expect<AST::FieldType> convertArrayField(Node N);
  Expect<ValType> convertStorageType(Node N);
  Expect<AST::FieldType> parseMutField(Node MutNode);

  // Section conversions
  Expect<AST::ImportDesc> convertInlineImport(Cursor &C, Cursor &FC);
  Expect<void> convertInlineExport(Cursor &C, Cursor &FC, ExternalType ExtType,
                                   uint32_t Idx);
  Expect<void> convertImport(Node N, AST::Module &Mod);
  Expect<void> convertExport(Node N, AST::Module &Mod);
  Expect<void> convertFunc(Node N, AST::Module &Mod);
  Expect<void> convertTable(Node N, AST::Module &Mod);
  Expect<void> convertMemory(Node N, AST::Module &Mod);
  Expect<void> convertGlobal(Node N, AST::Module &Mod);
  Expect<void> convertStart(Node N, AST::Module &Mod);
  Expect<void> convertElem(Node N, AST::Module &Mod);
  Expect<void> convertData(Node N, AST::Module &Mod);
  Expect<void> convertTag(Node N, AST::Module &Mod);

  // Instruction conversions
  Expect<void> convertExpression(Cursor &C, AST::Expression &Expr);
  Expect<void> convertMixedInstrSeq(Cursor &C, AST::InstrVec &Instrs);

  Expect<AST::Instruction> convertPlainInstr(Cursor &C, OpCode Code,
                                             uint32_t Offset);
  Expect<void> convertBlockInstr(Cursor &C, OpCode Code, uint32_t Offset,
                                 AST::InstrVec &Instrs);
  Expect<void> convertFoldedInstr(Node N, AST::InstrVec &Instrs,
                                  std::string_view *ErrorKeyword = nullptr);

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
  static NodeType nodeType(Node N);

  /// Give the node type at the current position of the cursor C.
  NodeType peekType(const Cursor &C) const;

  /// Return true if C is at a Sexpr, and its first keyword child is KW.
  bool sexprMatch(const Cursor &C, std::string_view KW) const;
  /// Return true if C is at a Sexpr, and its first keyword child is not KW.
  bool sexprUnmatch(const Cursor &C, std::string_view KW) const;

  Expect<uint32_t> resolveTypeUse(Cursor &C, AST::Module &Mod,
                                  bool CheckMismatch,
                                  bool AcceptParamId = true);
  Expect<AST::Limit> parseLimits(Cursor &C);
  Expect<AST::TableType> parseTabletype(Cursor &C);
  Expect<AST::GlobalType> parseGlobaltype(Cursor &C);
  Expect<BlockType> parseBlockType(Cursor &C);

  /// Find a function type that is equal to FuncTy. If there is no such type,
  /// make a new one. The function returns the type index.
  uint32_t findOrCreateFuncType(AST::FunctionType FuncTy, AST::Module &Mod);

  // State
  SymbolTable Syms;
  std::string_view Source;
  AST::Module *CurMod = nullptr;
  const Configure Conf;

  // The state of the import order.
  bool HasFuncDef = false;
  bool HasTableDef = false;
  bool HasMemoryDef = false;
  bool HasGlobalDef = false;
  bool HasStart = false;

  // The deferred inline exports. Pass 1 collects them, and pass 2 writes them.
  struct InlineExport {
    std::string Name;
    ExternalType Type;
    uint32_t Index;
  };
  std::vector<InlineExport> PendingExports;

  // The recursion guard for the depth of the folded instructions. The guard
  // prevents a native stack overflow on pathological input. This guard is only
  // for the recursive WAT converter, because the binary loader parses the
  // instructions with a loop.
  //
  // One level costs approximately 3 KiB of stack with optimization, and
  // approximately 5 KiB without optimization. The bound must hold for the
  // 1 MiB default thread stack of Windows with the worst codegen. A depth of
  // 64 levels needs less than 512 KiB at a pessimistic 8 KiB for each level.
  // The deepest folded nesting in the WebAssembly spec suite is 41.
  static constexpr uint32_t MaxInstrNestDepth = 64;
  uint32_t InstrNestDepth = 0;
};

} // namespace WAT
} // namespace WasmEdge
