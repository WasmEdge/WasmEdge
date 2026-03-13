// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "common/errcode.h"
#include "converter.h"

#include <algorithm>
#include <string>

using namespace std::string_view_literals;

namespace WasmEdge::WAT {

// ---------------------------------------------------------------------------
// resolveIndex
// ---------------------------------------------------------------------------
Expect<uint32_t> Converter::resolveIndex(
    Node N, const std::unordered_map<std::string, uint32_t, Hash::Hash> &Map,
    uint32_t Max, ErrCode::Value Err) {
  return Syms.resolveIdx(Map, nodeText(N), Max, Err);
}

// ---------------------------------------------------------------------------
// collectInlineExports
// ---------------------------------------------------------------------------
void Converter::collectInlineExports(Node N, ExternalType ExtType, uint32_t Idx,
                                     AST::Module & /*Mod*/) {
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) != "inline_export"sv) {
      continue;
    }
    for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
      Node NameNode = Child.namedChild(J);
      if (nodeType(NameNode) == "name"sv) {
        auto Bytes = parseString(nodeText(NameNode));
        std::string Name(reinterpret_cast<const char *>(Bytes.data()),
                         Bytes.size());
        PendingExports.push_back({std::move(Name), ExtType, Idx});
        break;
      }
    }
  }
}

// ---------------------------------------------------------------------------
// convertImport
// ---------------------------------------------------------------------------
Expect<void> Converter::convertImport(Node N, AST::Module &Mod) {
  AST::ImportDesc Desc;
  std::string ModName;
  std::string ExtName;
  Node DescNode;
  std::string_view DescType;

  uint32_t NameIdx = 0;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    auto Type = nodeType(Child);
    if (Type == "name"sv) {
      auto Bytes = parseString(nodeText(Child));
      std::string Name(reinterpret_cast<const char *>(Bytes.data()),
                       Bytes.size());
      if (NameIdx == 0) {
        ModName = std::move(Name);
      } else {
        ExtName = std::move(Name);
      }
      NameIdx++;
    } else if (Type == "import_func"sv || Type == "import_table"sv ||
               Type == "import_memory"sv || Type == "import_global"sv ||
               Type == "import_tag"sv) {
      DescNode = Child;
      DescType = Type;
    }
  }

  Desc.setModuleName(ModName);
  Desc.setExternalName(ExtName);

  auto resolveTypeUseOrInline = [&](Node DN) -> Expect<uint32_t> {
    // Look for type_use child
    for (uint32_t I = 0; I < DN.namedChildCount(); ++I) {
      Node Child = DN.namedChild(I);
      if (nodeType(Child) == "type_use"sv) {
        for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
          Node TChild = Child.namedChild(J);
          auto TType = nodeType(TChild);
          if (TType == "identifier"sv || TType == "nat"sv) {
            return Syms.resolveType(nodeText(TChild));
          }
        }
      }
    }
    // Build inline FunctionType from param/result
    AST::FunctionType FuncTy;
    for (uint32_t I = 0; I < DN.namedChildCount(); ++I) {
      Node Child = DN.namedChild(I);
      auto CT = nodeType(Child);
      if (CT == "param"sv) {
        for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
          Node PChild = Child.namedChild(J);
          if (nodeType(PChild) == "identifier"sv) {
            continue;
          }
          EXPECTED_TRY(auto VT, convertValType(PChild));
          FuncTy.getParamTypes().push_back(VT);
        }
      } else if (CT == "result"sv) {
        for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
          EXPECTED_TRY(auto VT, convertValType(Child.namedChild(J)));
          FuncTy.getReturnTypes().push_back(VT);
        }
      }
    }
    // Check existing types
    auto &Types = Mod.getTypeSection().getContent();
    for (uint32_t I = 0; I < Types.size(); ++I) {
      if (Types[I].getCompositeType().isFunc() &&
          Types[I].getCompositeType().getFuncType() == FuncTy) {
        return I;
      }
    }
    uint32_t NewIdx = static_cast<uint32_t>(Types.size());
    Types.emplace_back(std::move(FuncTy));
    return NewIdx;
  };

  auto parseLimits = [&](Node LN) -> Expect<AST::Limit> {
    std::vector<uint64_t> Nats;
    bool IsShared = false;
    for (uint32_t I = 0; I < LN.namedChildCount(); ++I) {
      Node Child = LN.namedChild(I);
      if (nodeType(Child) == "nat"sv) {
        EXPECTED_TRY(auto Val, parseUint(nodeText(Child)));
        Nats.push_back(Val);
      }
    }
    for (uint32_t I = 0; I < LN.childCount(); ++I) {
      if (nodeText(LN.child(I)) == "shared"sv) {
        IsShared = true;
        break;
      }
    }
    if (Nats.size() >= 2) {
      return AST::Limit(Nats[0], Nats[1], false, IsShared);
    } else if (Nats.size() == 1) {
      return AST::Limit(Nats[0]);
    }
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  };

  auto parseTableType = [&](Node TN) -> Expect<AST::TableType> {
    EXPECTED_TRY(auto Lim, parseLimits(TN));
    ValType RefTy(TypeCode::FuncRef);
    for (uint32_t J = 0; J < TN.namedChildCount(); ++J) {
      Node TChild = TN.namedChild(J);
      auto TType = nodeType(TChild);
      if (TType == "reftype"sv || TType == "ref_type_short"sv ||
          TType == "ref_type_full"sv) {
        EXPECTED_TRY(RefTy, convertRefType(TChild));
        break;
      }
    }
    return AST::TableType(RefTy, Lim);
  };

  auto parseGlobalType = [&](Node GN) -> Expect<AST::GlobalType> {
    auto GT = nodeType(GN);
    if (GT == "mut_type"sv) {
      for (uint32_t I = 0; I < GN.namedChildCount(); ++I) {
        EXPECTED_TRY(auto VT, convertValType(GN.namedChild(I)));
        return AST::GlobalType(VT, ValMut::Var);
      }
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(auto VT, convertValType(GN));
    return AST::GlobalType(VT, ValMut::Const);
  };

  if (DescType == "import_func"sv) {
    Desc.setExternalType(ExternalType::Function);
    EXPECTED_TRY(auto Idx, resolveTypeUseOrInline(DescNode));
    Desc.setExternalFuncTypeIdx(Idx);
  } else if (DescType == "import_table"sv) {
    Desc.setExternalType(ExternalType::Table);
    for (uint32_t I = 0; I < DescNode.namedChildCount(); ++I) {
      Node Child = DescNode.namedChild(I);
      if (nodeType(Child) == "table_type"sv) {
        EXPECTED_TRY(auto TT, parseTableType(Child));
        Desc.getExternalTableType() = TT;
        break;
      }
    }
  } else if (DescType == "import_memory"sv) {
    Desc.setExternalType(ExternalType::Memory);
    for (uint32_t I = 0; I < DescNode.namedChildCount(); ++I) {
      Node Child = DescNode.namedChild(I);
      if (nodeType(Child) == "memory_type"sv) {
        EXPECTED_TRY(auto Lim, parseLimits(Child));
        Desc.getExternalMemoryType() = AST::MemoryType(Lim);
        break;
      }
    }
  } else if (DescType == "import_global"sv) {
    Desc.setExternalType(ExternalType::Global);
    for (uint32_t I = 0; I < DescNode.namedChildCount(); ++I) {
      Node Child = DescNode.namedChild(I);
      if (nodeType(Child) == "identifier"sv) {
        continue;
      }
      EXPECTED_TRY(auto GT, parseGlobalType(Child));
      Desc.getExternalGlobalType() = GT;
      break;
    }
  } else if (DescType == "import_tag"sv) {
    Desc.setExternalType(ExternalType::Tag);
    EXPECTED_TRY(auto Idx, resolveTypeUseOrInline(DescNode));
    Desc.getExternalTagType().setTypeIdx(Idx);
  }

  Mod.getImportSection().getContent().push_back(std::move(Desc));
  return {};
}

// ---------------------------------------------------------------------------
// convertExport
// ---------------------------------------------------------------------------
Expect<void> Converter::convertExport(Node N, AST::Module &Mod) {
  AST::ExportDesc Desc;

  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) == "name"sv) {
      auto Bytes = parseString(nodeText(Child));
      std::string Name(reinterpret_cast<const char *>(Bytes.data()),
                       Bytes.size());
      Desc.setExternalName(Name);
      break;
    }
  }

  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) != "export_desc"sv) {
      continue;
    }
    ExternalType ExtType = ExternalType::Function;
    for (uint32_t J = 0; J < Child.childCount(); ++J) {
      Node DChild = Child.child(J);
      auto Text = nodeText(DChild);
      if (Text == "func"sv) {
        ExtType = ExternalType::Function;
      } else if (Text == "table"sv) {
        ExtType = ExternalType::Table;
      } else if (Text == "memory"sv) {
        ExtType = ExternalType::Memory;
      } else if (Text == "global"sv) {
        ExtType = ExternalType::Global;
      } else if (Text == "tag"sv) {
        ExtType = ExternalType::Tag;
      }
    }
    Desc.setExternalType(ExtType);

    for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
      Node IdxNode = Child.namedChild(J);
      auto IT = nodeType(IdxNode);
      if (IT == "identifier"sv || IT == "nat"sv) {
        uint32_t Idx = 0;
        auto IdxText = nodeText(IdxNode);
        if (ExtType == ExternalType::Function) {
          EXPECTED_TRY(Idx, Syms.resolveFunc(IdxText));
        } else if (ExtType == ExternalType::Table) {
          EXPECTED_TRY(Idx, Syms.resolveTable(IdxText));
        } else if (ExtType == ExternalType::Memory) {
          EXPECTED_TRY(Idx, Syms.resolveMemory(IdxText));
        } else if (ExtType == ExternalType::Global) {
          EXPECTED_TRY(Idx, Syms.resolveGlobal(IdxText));
        } else if (ExtType == ExternalType::Tag) {
          EXPECTED_TRY(Idx, Syms.resolveTag(IdxText));
        }
        Desc.setExternalIndex(Idx);
        break;
      }
    }
    break;
  }

  Mod.getExportSection().getContent().push_back(std::move(Desc));
  return {};
}

// ---------------------------------------------------------------------------
// convertFunc
// ---------------------------------------------------------------------------
Expect<void> Converter::convertFunc(Node N, AST::Module &Mod) {
  // Resolve inline type helper (reused in several places)
  auto resolveTypeUseOrInline = [&](Node DN) -> Expect<uint32_t> {
    for (uint32_t I = 0; I < DN.namedChildCount(); ++I) {
      Node Child = DN.namedChild(I);
      if (nodeType(Child) == "type_use"sv) {
        for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
          Node TChild = Child.namedChild(J);
          auto TType = nodeType(TChild);
          if (TType == "identifier"sv || TType == "nat"sv) {
            return Syms.resolveType(nodeText(TChild));
          }
        }
      }
    }
    AST::FunctionType FuncTy;
    for (uint32_t I = 0; I < DN.namedChildCount(); ++I) {
      Node Child = DN.namedChild(I);
      auto CT = nodeType(Child);
      if (CT == "param"sv) {
        for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
          Node PChild = Child.namedChild(J);
          if (nodeType(PChild) == "identifier"sv) {
            continue;
          }
          EXPECTED_TRY(auto VT, convertValType(PChild));
          FuncTy.getParamTypes().push_back(VT);
        }
      } else if (CT == "result"sv) {
        for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
          EXPECTED_TRY(auto VT, convertValType(Child.namedChild(J)));
          FuncTy.getReturnTypes().push_back(VT);
        }
      }
    }
    auto &Types = Mod.getTypeSection().getContent();
    for (uint32_t I = 0; I < Types.size(); ++I) {
      if (Types[I].getCompositeType().isFunc() &&
          Types[I].getCompositeType().getFuncType() == FuncTy) {
        return I;
      }
    }
    uint32_t NewIdx = static_cast<uint32_t>(Types.size());
    Types.emplace_back(std::move(FuncTy));
    return NewIdx;
  };

  // Calculate function index
  uint32_t ImportedFuncs = 0;
  for (const auto &Imp : Mod.getImportSection().getContent()) {
    if (Imp.getExternalType() == ExternalType::Function) {
      ImportedFuncs++;
    }
  }
  uint32_t FuncIdx =
      ImportedFuncs +
      static_cast<uint32_t>(Mod.getFunctionSection().getContent().size());

  // Check for inline import
  bool HasInlineImport = false;
  Node InlineImportNode;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) == "inline_import"sv) {
      HasInlineImport = true;
      InlineImportNode = Child;
      break;
    }
  }

  collectInlineExports(N, ExternalType::Function, FuncIdx, Mod);

  if (HasInlineImport) {
    AST::ImportDesc Desc;
    Desc.setExternalType(ExternalType::Function);

    uint32_t NameCount = 0;
    for (uint32_t I = 0; I < InlineImportNode.namedChildCount(); ++I) {
      Node Child = InlineImportNode.namedChild(I);
      if (nodeType(Child) == "name"sv) {
        auto Bytes = parseString(nodeText(Child));
        std::string Name(reinterpret_cast<const char *>(Bytes.data()),
                         Bytes.size());
        if (NameCount == 0) {
          Desc.setModuleName(Name);
        } else {
          Desc.setExternalName(Name);
        }
        NameCount++;
      }
    }

    EXPECTED_TRY(auto Idx, resolveTypeUseOrInline(N));
    Desc.setExternalFuncTypeIdx(Idx);
    Mod.getImportSection().getContent().push_back(std::move(Desc));
    return {};
  }

  // Regular function definition
  EXPECTED_TRY(auto TypeIdx, resolveTypeUseOrInline(N));

  // Clear per-function locals
  Syms.clearLocals();

  // Register param identifiers as locals
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) == "param"sv) {
      for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
        Node PChild = Child.namedChild(J);
        if (nodeType(PChild) == "identifier"sv) {
          std::string Id(nodeText(PChild));
          Syms.Locals[Id] = Syms.NextLocal;
        } else {
          Syms.NextLocal++;
        }
      }
    }
  }

  // Parse local declarations and build CodeSegment
  AST::CodeSegment CodeSeg;

  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) != "local"sv) {
      continue;
    }
    std::string LocalId;
    for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
      Node LChild = Child.namedChild(J);
      if (nodeType(LChild) == "identifier"sv) {
        LocalId = std::string(nodeText(LChild));
      } else {
        EXPECTED_TRY(auto VT, convertValType(LChild));
        if (!LocalId.empty()) {
          Syms.Locals[LocalId] = Syms.NextLocal;
          LocalId.clear();
        }
        CodeSeg.getLocals().emplace_back(1, VT);
        Syms.NextLocal++;
      }
    }
  }

  // Parse body instructions
  auto &Instrs = CodeSeg.getExpr().getInstrs();
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    auto Type = nodeType(Child);
    if (Type == "plain_instr"sv || Type == "block_instr"sv ||
        Type == "block_block"sv || Type == "block_loop"sv ||
        Type == "block_if"sv || Type == "block_try_table"sv ||
        Type == "folded_instr"sv) {
      EXPECTED_TRY(convertInstr(Child, Instrs));
    }
  }
  Instrs.emplace_back(OpCode::End);
  Instrs.back().setExprLast(true);

  Mod.getFunctionSection().getContent().push_back(TypeIdx);
  Mod.getCodeSection().getContent().push_back(std::move(CodeSeg));
  return {};
}

// ---------------------------------------------------------------------------
// convertTable
// ---------------------------------------------------------------------------
Expect<void> Converter::convertTable(Node N, AST::Module &Mod) {
  auto parseLimits = [&](Node LN) -> Expect<AST::Limit> {
    std::vector<uint64_t> Nats;
    bool IsShared = false;
    for (uint32_t I = 0; I < LN.namedChildCount(); ++I) {
      Node Child = LN.namedChild(I);
      if (nodeType(Child) == "nat"sv) {
        EXPECTED_TRY(auto Val, parseUint(nodeText(Child)));
        Nats.push_back(Val);
      }
    }
    for (uint32_t I = 0; I < LN.childCount(); ++I) {
      if (nodeText(LN.child(I)) == "shared"sv) {
        IsShared = true;
        break;
      }
    }
    if (Nats.size() >= 2) {
      return AST::Limit(Nats[0], Nats[1], false, IsShared);
    } else if (Nats.size() == 1) {
      return AST::Limit(Nats[0]);
    }
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  };

  auto parseTableType = [&](Node TN) -> Expect<AST::TableType> {
    EXPECTED_TRY(auto Lim, parseLimits(TN));
    ValType RefTy(TypeCode::FuncRef);
    for (uint32_t J = 0; J < TN.namedChildCount(); ++J) {
      Node TChild = TN.namedChild(J);
      auto TType = nodeType(TChild);
      if (TType == "reftype"sv || TType == "ref_type_short"sv ||
          TType == "ref_type_full"sv) {
        EXPECTED_TRY(RefTy, convertRefType(TChild));
        break;
      }
    }
    return AST::TableType(RefTy, Lim);
  };

  uint32_t TableIdx = 0;
  for (const auto &Imp : Mod.getImportSection().getContent()) {
    if (Imp.getExternalType() == ExternalType::Table) {
      TableIdx++;
    }
  }
  TableIdx += static_cast<uint32_t>(Mod.getTableSection().getContent().size());

  bool HasInlineImport = false;
  Node InlineImportNode;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) == "inline_import"sv) {
      HasInlineImport = true;
      InlineImportNode = Child;
      break;
    }
  }

  collectInlineExports(N, ExternalType::Table, TableIdx, Mod);

  if (HasInlineImport) {
    AST::ImportDesc Desc;
    Desc.setExternalType(ExternalType::Table);
    uint32_t NameCount = 0;
    for (uint32_t I = 0; I < InlineImportNode.namedChildCount(); ++I) {
      Node Child = InlineImportNode.namedChild(I);
      if (nodeType(Child) == "name"sv) {
        auto Bytes = parseString(nodeText(Child));
        std::string Name(reinterpret_cast<const char *>(Bytes.data()),
                         Bytes.size());
        if (NameCount == 0) {
          Desc.setModuleName(Name);
        } else {
          Desc.setExternalName(Name);
        }
        NameCount++;
      }
    }
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) == "table_type"sv) {
        EXPECTED_TRY(auto TT, parseTableType(Child));
        Desc.getExternalTableType() = TT;
        break;
      }
    }
    Mod.getImportSection().getContent().push_back(std::move(Desc));
    return {};
  }

  // Check for table_type vs inline elem
  bool HasTableType = false;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    if (nodeType(N.namedChild(I)) == "table_type"sv) {
      HasTableType = true;
      break;
    }
  }

  if (HasTableType) {
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) == "table_type"sv) {
        EXPECTED_TRY(auto TT, parseTableType(Child));
        AST::TableSegment Seg;
        Seg.getTableType() = TT;
        Mod.getTableSection().getContent().push_back(std::move(Seg));
        break;
      }
    }
  } else {
    // Inline elem: reftype (elem elem_item*)
    ValType RefTy(TypeCode::FuncRef);
    std::vector<AST::Expression> InitExprs;

    bool FoundRefType = false;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      auto Type = nodeType(Child);
      if (Type == "reftype"sv || Type == "ref_type_short"sv ||
          Type == "ref_type_full"sv) {
        EXPECTED_TRY(RefTy, convertRefType(Child));
        FoundRefType = true;
      } else if (FoundRefType &&
                 (Type == "elem_item"sv || Type == "_elem_item"sv)) {
        AST::Expression Expr;
        EXPECTED_TRY(convertExpression(Child, Expr));
        InitExprs.push_back(std::move(Expr));
      } else if (FoundRefType && (Type == "identifier"sv || Type == "nat"sv)) {
        EXPECTED_TRY(auto Idx, Syms.resolveFunc(nodeText(Child)));
        AST::Expression Expr;
        Expr.getInstrs().emplace_back(OpCode::Ref__func);
        Expr.getInstrs().back().getTargetIndex() = Idx;
        Expr.getInstrs().emplace_back(OpCode::End);
        Expr.getInstrs().back().setExprLast(true);
        InitExprs.push_back(std::move(Expr));
      }
    }

    uint32_t ElemCount = static_cast<uint32_t>(InitExprs.size());

    AST::TableSegment TSeg;
    TSeg.getTableType() = AST::TableType(RefTy, ElemCount, ElemCount);
    Mod.getTableSection().getContent().push_back(std::move(TSeg));

    AST::ElementSegment ESeg;
    ESeg.setMode(AST::ElementSegment::ElemMode::Active);
    ESeg.setRefType(RefTy);
    ESeg.setIdx(TableIdx);
    ESeg.getExpr().getInstrs().emplace_back(OpCode::I32__const);
    ESeg.getExpr().getInstrs().back().setNum(
        static_cast<uint128_t>(static_cast<uint32_t>(0)));
    ESeg.getExpr().getInstrs().emplace_back(OpCode::End);
    ESeg.getExpr().getInstrs().back().setExprLast(true);
    ESeg.getInitExprs() = std::move(InitExprs);
    Mod.getElementSection().getContent().push_back(std::move(ESeg));
  }

  return {};
}

// ---------------------------------------------------------------------------
// convertMemory
// ---------------------------------------------------------------------------
Expect<void> Converter::convertMemory(Node N, AST::Module &Mod) {
  auto parseLimits = [&](Node LN) -> Expect<AST::Limit> {
    std::vector<uint64_t> Nats;
    bool IsShared = false;
    for (uint32_t I = 0; I < LN.namedChildCount(); ++I) {
      Node Child = LN.namedChild(I);
      if (nodeType(Child) == "nat"sv) {
        EXPECTED_TRY(auto Val, parseUint(nodeText(Child)));
        Nats.push_back(Val);
      }
    }
    for (uint32_t I = 0; I < LN.childCount(); ++I) {
      if (nodeText(LN.child(I)) == "shared"sv) {
        IsShared = true;
        break;
      }
    }
    if (Nats.size() >= 2) {
      return AST::Limit(Nats[0], Nats[1], false, IsShared);
    } else if (Nats.size() == 1) {
      return AST::Limit(Nats[0]);
    }
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  };

  uint32_t MemIdx = 0;
  for (const auto &Imp : Mod.getImportSection().getContent()) {
    if (Imp.getExternalType() == ExternalType::Memory) {
      MemIdx++;
    }
  }
  MemIdx += static_cast<uint32_t>(Mod.getMemorySection().getContent().size());

  bool HasInlineImport = false;
  Node InlineImportNode;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) == "inline_import"sv) {
      HasInlineImport = true;
      InlineImportNode = Child;
      break;
    }
  }

  collectInlineExports(N, ExternalType::Memory, MemIdx, Mod);

  if (HasInlineImport) {
    AST::ImportDesc Desc;
    Desc.setExternalType(ExternalType::Memory);
    uint32_t NameCount = 0;
    for (uint32_t I = 0; I < InlineImportNode.namedChildCount(); ++I) {
      Node Child = InlineImportNode.namedChild(I);
      if (nodeType(Child) == "name"sv) {
        auto Bytes = parseString(nodeText(Child));
        std::string Name(reinterpret_cast<const char *>(Bytes.data()),
                         Bytes.size());
        if (NameCount == 0) {
          Desc.setModuleName(Name);
        } else {
          Desc.setExternalName(Name);
        }
        NameCount++;
      }
    }
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) == "memory_type"sv) {
        EXPECTED_TRY(auto Lim, parseLimits(Child));
        Desc.getExternalMemoryType() = AST::MemoryType(Lim);
        break;
      }
    }
    Mod.getImportSection().getContent().push_back(std::move(Desc));
    return {};
  }

  bool HasMemoryType = false;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    if (nodeType(N.namedChild(I)) == "memory_type"sv) {
      HasMemoryType = true;
      break;
    }
  }

  if (HasMemoryType) {
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) == "memory_type"sv) {
        EXPECTED_TRY(auto Lim, parseLimits(Child));
        Mod.getMemorySection().getContent().emplace_back(Lim);
        break;
      }
    }
  } else {
    // Inline data
    std::vector<Byte> DataBytes;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) == "string"sv) {
        auto Bytes = parseString(nodeText(Child));
        DataBytes.insert(DataBytes.end(), Bytes.begin(), Bytes.end());
      }
    }

    const uint64_t PageSize = 65536;
    uint64_t DataSize = DataBytes.size();
    uint64_t Pages = (DataSize + PageSize - 1) / PageSize;

    Mod.getMemorySection().getContent().emplace_back(Pages, Pages);

    AST::DataSegment DSeg;
    DSeg.setMode(AST::DataSegment::DataMode::Active);
    DSeg.setIdx(MemIdx);
    DSeg.getExpr().getInstrs().emplace_back(OpCode::I32__const);
    DSeg.getExpr().getInstrs().back().setNum(
        static_cast<uint128_t>(static_cast<uint32_t>(0)));
    DSeg.getExpr().getInstrs().emplace_back(OpCode::End);
    DSeg.getExpr().getInstrs().back().setExprLast(true);
    DSeg.getData() = std::move(DataBytes);
    Mod.getDataSection().getContent().push_back(std::move(DSeg));
  }

  return {};
}

// ---------------------------------------------------------------------------
// convertGlobal
// ---------------------------------------------------------------------------
Expect<void> Converter::convertGlobal(Node N, AST::Module &Mod) {
  auto parseGlobalType = [&](Node GN) -> Expect<AST::GlobalType> {
    auto GT = nodeType(GN);
    if (GT == "mut_type"sv) {
      for (uint32_t I = 0; I < GN.namedChildCount(); ++I) {
        EXPECTED_TRY(auto VT, convertValType(GN.namedChild(I)));
        return AST::GlobalType(VT, ValMut::Var);
      }
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(auto VT, convertValType(GN));
    return AST::GlobalType(VT, ValMut::Const);
  };

  uint32_t GlobalIdx = 0;
  for (const auto &Imp : Mod.getImportSection().getContent()) {
    if (Imp.getExternalType() == ExternalType::Global) {
      GlobalIdx++;
    }
  }
  GlobalIdx +=
      static_cast<uint32_t>(Mod.getGlobalSection().getContent().size());

  bool HasInlineImport = false;
  Node InlineImportNode;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) == "inline_import"sv) {
      HasInlineImport = true;
      InlineImportNode = Child;
      break;
    }
  }

  collectInlineExports(N, ExternalType::Global, GlobalIdx, Mod);

  if (HasInlineImport) {
    AST::ImportDesc Desc;
    Desc.setExternalType(ExternalType::Global);
    uint32_t NameCount = 0;
    for (uint32_t I = 0; I < InlineImportNode.namedChildCount(); ++I) {
      Node Child = InlineImportNode.namedChild(I);
      if (nodeType(Child) == "name"sv) {
        auto Bytes = parseString(nodeText(Child));
        std::string Name(reinterpret_cast<const char *>(Bytes.data()),
                         Bytes.size());
        if (NameCount == 0) {
          Desc.setModuleName(Name);
        } else {
          Desc.setExternalName(Name);
        }
        NameCount++;
      }
    }
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      auto CT = nodeType(Child);
      if (CT == "identifier"sv || CT == "inline_export"sv ||
          CT == "inline_import"sv) {
        continue;
      }
      EXPECTED_TRY(auto GT, parseGlobalType(Child));
      Desc.getExternalGlobalType() = GT;
      break;
    }
    Mod.getImportSection().getContent().push_back(std::move(Desc));
    return {};
  }

  AST::GlobalSegment Seg;

  bool FoundGlobalType = false;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    auto CT = nodeType(Child);
    if (CT == "identifier"sv || CT == "inline_export"sv ||
        CT == "inline_import"sv) {
      continue;
    }
    if (!FoundGlobalType) {
      EXPECTED_TRY(auto GT, parseGlobalType(Child));
      Seg.getGlobalType() = GT;
      FoundGlobalType = true;
      continue;
    }
    break;
  }

  auto &Instrs = Seg.getExpr().getInstrs();
  bool PastGlobalType = false;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    auto Type = nodeType(Child);
    if (Type == "identifier"sv || Type == "inline_export"sv ||
        Type == "inline_import"sv) {
      continue;
    }
    if (!PastGlobalType) {
      PastGlobalType = true;
      continue;
    }
    if (Type == "plain_instr"sv || Type == "block_instr"sv ||
        Type == "block_block"sv || Type == "block_loop"sv ||
        Type == "block_if"sv || Type == "block_try_table"sv ||
        Type == "folded_instr"sv) {
      EXPECTED_TRY(convertInstr(Child, Instrs));
    }
  }
  Instrs.emplace_back(OpCode::End);
  Instrs.back().setExprLast(true);

  Mod.getGlobalSection().getContent().push_back(std::move(Seg));
  return {};
}

// ---------------------------------------------------------------------------
// convertStart
// ---------------------------------------------------------------------------
Expect<void> Converter::convertStart(Node N, AST::Module &Mod) {
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    auto Type = nodeType(Child);
    if (Type == "identifier"sv || Type == "nat"sv) {
      EXPECTED_TRY(auto Idx, Syms.resolveFunc(nodeText(Child)));
      Mod.getStartSection().setContent(Idx);
      return {};
    }
  }
  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

// ---------------------------------------------------------------------------
// convertElem
// ---------------------------------------------------------------------------
Expect<void> Converter::convertElem(Node N, AST::Module &Mod) {
  AST::ElementSegment Seg;
  Seg.setMode(AST::ElementSegment::ElemMode::Passive);
  Seg.setRefType(ValType(TypeCode::FuncRef));

  bool FuncKeyword = false;

  // Scan anonymous children for keywords first
  for (uint32_t I = 0; I < N.childCount(); ++I) {
    auto Text = nodeText(N.child(I));
    if (Text == "declare"sv) {
      Seg.setMode(AST::ElementSegment::ElemMode::Declarative);
    } else if (Text == "func"sv) {
      FuncKeyword = true;
    }
  }

  bool SeenElemId = false;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    auto Type = nodeType(Child);

    if (Type == "identifier"sv) {
      if (!SeenElemId) {
        SeenElemId = true;
        continue;
      }
      if (FuncKeyword) {
        EXPECTED_TRY(auto Idx, Syms.resolveFunc(nodeText(Child)));
        AST::Expression Expr;
        Expr.getInstrs().emplace_back(OpCode::Ref__func);
        Expr.getInstrs().back().getTargetIndex() = Idx;
        Expr.getInstrs().emplace_back(OpCode::End);
        Expr.getInstrs().back().setExprLast(true);
        Seg.getInitExprs().push_back(std::move(Expr));
      }
    } else if (Type == "_table_use"sv || Type == "table_use"sv) {
      Seg.setMode(AST::ElementSegment::ElemMode::Active);
      for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
        Node TChild = Child.namedChild(J);
        auto TType = nodeType(TChild);
        if (TType == "identifier"sv || TType == "nat"sv) {
          EXPECTED_TRY(auto Idx, Syms.resolveTable(nodeText(TChild)));
          Seg.setIdx(Idx);
          break;
        }
      }
    } else if (Type == "_offset_expr"sv || Type == "offset_expr"sv) {
      Seg.setMode(AST::ElementSegment::ElemMode::Active);
      EXPECTED_TRY(convertExpression(Child, Seg.getExpr()));
    } else if (Type == "reftype"sv || Type == "ref_type_short"sv ||
               Type == "ref_type_full"sv) {
      EXPECTED_TRY(auto RefTy, convertRefType(Child));
      Seg.setRefType(RefTy);
    } else if (Type == "elem_item"sv || Type == "_elem_item"sv) {
      AST::Expression Expr;
      EXPECTED_TRY(convertExpression(Child, Expr));
      Seg.getInitExprs().push_back(std::move(Expr));
    } else if (Type == "nat"sv) {
      EXPECTED_TRY(auto Idx, Syms.resolveFunc(nodeText(Child)));
      AST::Expression Expr;
      Expr.getInstrs().emplace_back(OpCode::Ref__func);
      Expr.getInstrs().back().getTargetIndex() = Idx;
      Expr.getInstrs().emplace_back(OpCode::End);
      Expr.getInstrs().back().setExprLast(true);
      Seg.getInitExprs().push_back(std::move(Expr));
    }
  }

  Mod.getElementSection().getContent().push_back(std::move(Seg));
  return {};
}

// ---------------------------------------------------------------------------
// convertData
// ---------------------------------------------------------------------------
Expect<void> Converter::convertData(Node N, AST::Module &Mod) {
  AST::DataSegment Seg;
  Seg.setMode(AST::DataSegment::DataMode::Passive);

  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    auto Type = nodeType(Child);

    if (Type == "identifier"sv) {
      continue;
    } else if (Type == "_memory_use"sv || Type == "memory_use"sv) {
      for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
        Node MChild = Child.namedChild(J);
        auto MType = nodeType(MChild);
        if (MType == "identifier"sv || MType == "nat"sv) {
          EXPECTED_TRY(auto Idx, Syms.resolveMemory(nodeText(MChild)));
          Seg.setIdx(Idx);
          break;
        }
      }
    } else if (Type == "_offset_expr"sv || Type == "offset_expr"sv) {
      Seg.setMode(AST::DataSegment::DataMode::Active);
      EXPECTED_TRY(convertExpression(Child, Seg.getExpr()));
    } else if (Type == "string"sv) {
      auto Bytes = parseString(nodeText(Child));
      auto &Data = Seg.getData();
      Data.insert(Data.end(), Bytes.begin(), Bytes.end());
    }
  }

  Mod.getDataSection().getContent().push_back(std::move(Seg));
  return {};
}

// ---------------------------------------------------------------------------
// convertTag
// ---------------------------------------------------------------------------
Expect<void> Converter::convertTag(Node N, AST::Module &Mod) {
  auto resolveTypeUseOrInline = [&](Node DN) -> Expect<uint32_t> {
    for (uint32_t I = 0; I < DN.namedChildCount(); ++I) {
      Node Child = DN.namedChild(I);
      if (nodeType(Child) == "type_use"sv) {
        for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
          Node TChild = Child.namedChild(J);
          auto TType = nodeType(TChild);
          if (TType == "identifier"sv || TType == "nat"sv) {
            return Syms.resolveType(nodeText(TChild));
          }
        }
      }
    }
    AST::FunctionType FuncTy;
    for (uint32_t I = 0; I < DN.namedChildCount(); ++I) {
      Node Child = DN.namedChild(I);
      auto CT = nodeType(Child);
      if (CT == "param"sv) {
        for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
          Node PChild = Child.namedChild(J);
          if (nodeType(PChild) == "identifier"sv) {
            continue;
          }
          EXPECTED_TRY(auto VT, convertValType(PChild));
          FuncTy.getParamTypes().push_back(VT);
        }
      } else if (CT == "result"sv) {
        for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
          EXPECTED_TRY(auto VT, convertValType(Child.namedChild(J)));
          FuncTy.getReturnTypes().push_back(VT);
        }
      }
    }
    auto &Types = Mod.getTypeSection().getContent();
    for (uint32_t I = 0; I < Types.size(); ++I) {
      if (Types[I].getCompositeType().isFunc() &&
          Types[I].getCompositeType().getFuncType() == FuncTy) {
        return I;
      }
    }
    uint32_t NewIdx = static_cast<uint32_t>(Types.size());
    Types.emplace_back(std::move(FuncTy));
    return NewIdx;
  };

  uint32_t TagIdx = 0;
  for (const auto &Imp : Mod.getImportSection().getContent()) {
    if (Imp.getExternalType() == ExternalType::Tag) {
      TagIdx++;
    }
  }
  TagIdx += static_cast<uint32_t>(Mod.getTagSection().getContent().size());

  bool HasInlineImport = false;
  Node InlineImportNode;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) == "inline_import"sv) {
      HasInlineImport = true;
      InlineImportNode = Child;
      break;
    }
  }

  collectInlineExports(N, ExternalType::Tag, TagIdx, Mod);

  if (HasInlineImport) {
    AST::ImportDesc Desc;
    Desc.setExternalType(ExternalType::Tag);
    uint32_t NameCount = 0;
    for (uint32_t I = 0; I < InlineImportNode.namedChildCount(); ++I) {
      Node Child = InlineImportNode.namedChild(I);
      if (nodeType(Child) == "name"sv) {
        auto Bytes = parseString(nodeText(Child));
        std::string Name(reinterpret_cast<const char *>(Bytes.data()),
                         Bytes.size());
        if (NameCount == 0) {
          Desc.setModuleName(Name);
        } else {
          Desc.setExternalName(Name);
        }
        NameCount++;
      }
    }
    EXPECTED_TRY(auto Idx, resolveTypeUseOrInline(N));
    Desc.getExternalTagType().setTypeIdx(Idx);
    Mod.getImportSection().getContent().push_back(std::move(Desc));
    return {};
  }

  AST::TagType Tag;
  EXPECTED_TRY(auto Idx, resolveTypeUseOrInline(N));
  Tag.setTypeIdx(Idx);
  Mod.getTagSection().getContent().push_back(std::move(Tag));
  return {};
}

} // namespace WasmEdge::WAT
