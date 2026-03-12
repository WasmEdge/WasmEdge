// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#pragma once

#include "tree_sitter.h"

#include "ast/module.h"
#include "common/errcode.h"
#include "common/hash.h"
#include "common/types.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace WasmEdge::WAT {

class Converter {
public:
  Converter() = default;

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
    std::vector<std::string> Labels;

    uint32_t NextType = 0, NextFunc = 0, NextTable = 0, NextMemory = 0;
    uint32_t NextGlobal = 0, NextTag = 0, NextElem = 0, NextData = 0;
    uint32_t NextLocal = 0;

    void clear();
    void clearLocals();

    Expect<uint32_t>
    resolveIdx(const std::unordered_map<std::string, uint32_t, Hash::Hash> &Map,
               std::string_view Ref, uint32_t Max, ErrCode::Value Err) const;

    Expect<uint32_t> resolveType(std::string_view Ref) const;
    Expect<uint32_t> resolveFunc(std::string_view Ref) const;
    Expect<uint32_t> resolveTable(std::string_view Ref) const;
    Expect<uint32_t> resolveMemory(std::string_view Ref) const;
    Expect<uint32_t> resolveGlobal(std::string_view Ref) const;
    Expect<uint32_t> resolveTag(std::string_view Ref) const;
    Expect<uint32_t> resolveElem(std::string_view Ref) const;
    Expect<uint32_t> resolveData(std::string_view Ref) const;
    Expect<uint32_t> resolveLocal(std::string_view Ref) const;
    Expect<uint32_t> resolveLabel(std::string_view Ref) const;
  };

  // --- Pass 1: Collect identifiers and assign indices ---
  Expect<void> collectIndices(Node Root);
  Expect<void> collectModuleField(Node Field);

  // --- Pass 2: Build AST sections ---
  Expect<void> buildModule(Node Root, AST::Module &Mod);
  Expect<void> buildModuleField(Node Field, AST::Module &Mod);

  // Type conversions
  Expect<void> convertTypeDefinition(Node N, AST::Module &Mod);
  Expect<void> convertRecType(Node N, AST::Module &Mod);
  Expect<AST::SubType> convertSubType(Node N);
  Expect<AST::FunctionType> convertFuncType(Node N);
  Expect<ValType> convertValType(Node N);
  Expect<ValType> convertRefType(Node N);
  Expect<ValType> convertHeapType(Node N);
  Expect<std::vector<AST::FieldType>> convertStructFields(Node N);
  Expect<AST::FieldType> convertArrayField(Node N);
  Expect<ValType> convertStorageType(Node N);

  // Section conversions
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
  Expect<void> convertExpression(Node N, AST::Expression &Expr);
  Expect<void> convertInstr(Node N, AST::InstrVec &Instrs);
  Expect<void> convertPlainInstr(Node N, AST::InstrVec &Instrs);
  Expect<void> convertBlockInstr(Node N, AST::InstrVec &Instrs);
  Expect<void> convertFoldedInstr(Node N, AST::InstrVec &Instrs);

  // Literal parsing
  Expect<uint64_t> parseUint(std::string_view Text);
  Expect<int64_t> parseInt(std::string_view Text);
  Expect<float> parseF32(std::string_view Text);
  Expect<double> parseF64(std::string_view Text);
  std::vector<Byte> parseString(std::string_view Text);

  // Helpers
  std::string_view nodeText(Node N) const;
  std::string_view nodeType(Node N) const;
  Expect<uint32_t>
  resolveIndex(Node N,
               const std::unordered_map<std::string, uint32_t, Hash::Hash> &Map,
               uint32_t Max, ErrCode::Value Err);
  void collectInlineExports(Node N, ExternalType ExtType, uint32_t Idx,
                            AST::Module &Mod);

  // State
  SymbolTable Syms;
  std::string_view Source;

  // Deferred inline exports (collected in pass 1, emitted in pass 2)
  struct InlineExport {
    std::string Name;
    ExternalType Type;
    uint32_t Index;
  };
  std::vector<InlineExport> PendingExports;

  // Deferred inline imports
  struct InlineImport {
    std::string ModName;
    std::string ExtName;
  };
};

} // namespace WasmEdge::WAT
