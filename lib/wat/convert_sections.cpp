// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/errcode.h"
#include "common/errinfo.h"
#include "common/spdlog.h"
#include "converter.h"
#include "wat/wat_util.h"

#include <algorithm>
#include <limits>
#include <string>

using namespace std::string_view_literals;

namespace WasmEdge {
namespace WAT {

// typeuse ::= (type typeidx) (param valtype*)* (result valtype*)*
// Resolve an explicit (type $idx), or an inline list of parameters and
// results, into a type index.
Expect<uint32_t> Converter::resolveTypeUse(Cursor &C, AST::Module &Mod,
                                           bool AcceptParamId) {
  AST::FunctionType FuncTy;
  uint32_t TypeUseIdx = 0;
  bool HasType = false;
  bool HasParam = false;
  bool HasResult = false;
  while (C.valid()) {
    Node Child = C.node();
    if (nodeType(Child) == NodeType::Sexpr) {
      Cursor FC(Child);
      if (peekType(FC) != NodeType::Keyword) {
        break;
      }
      auto Keyword = nodeText(FC.node());
      FC.next();
      if (Keyword == "type"sv) {
        // A typeuse holds at most one (type ...), and it comes first.
        if (HasType || HasParam || HasResult) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        HasType = true;
        // This is the type index. The child is an Id or a number, and it is
        // the only child.
        if (!FC.valid()) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        Node TChild = FC.node();
        auto TType = nodeType(TChild);
        if (TType != NodeType::Id && TType != NodeType::U) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        EXPECTED_TRY(TypeUseIdx, Syms.resolveType(nodeText(TChild)));
        FC.next();
        if (FC.valid()) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
      } else if (Keyword == "param"sv) {
        HasParam = true;
        if (HasResult) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        EXPECTED_TRY(
            parseParamList(FC, FuncTy.getParamTypes(), AcceptParamId, nullptr));
      } else if (Keyword == "result"sv) {
        HasResult = true;
        while (FC.valid()) {
          EXPECTED_TRY(auto VT, convertValType(FC.node()));
          FuncTy.getReturnTypes().push_back(VT);
          FC.next();
        }
      } else {
        break;
      }
      C.next();
    } else {
      break;
    }
  }
  if (HasType) {
    auto &Types = Mod.getTypeSection().getContent();
    if (TypeUseIdx < Types.size()) {
      // A typeuse must give a function type. Reject a GC struct type or array
      // type that is already complete. The calls to getFuncType() in this
      // function and in a caller such as convertFunc must not dereference a
      // null FunctionType variant.
      if (!Types[TypeUseIdx].getCompositeType().isFunc()) {
        return Unexpect(ErrCode::Value::WatUnknownType);
      }
      // If the field has a type_use and an inline type, the two types must be
      // equal.
      if (HasParam || HasResult) {
        const auto &RefTy = Types[TypeUseIdx].getCompositeType().getFuncType();
        if (FuncTy.getParamTypes() != RefTy.getParamTypes() ||
            FuncTy.getReturnTypes() != RefTy.getReturnTypes()) {
          return Unexpect(ErrCode::Value::WatInlineFuncType);
        }
      }
    } else if (HasParam || HasResult) {
      // An inline type needs the type at this point for the comparison. As a
      // result, a numeric index that is out of range is a syntax error here.
      // Without an inline type, the validator reports the index, as it does
      // for a binary module.
      return Unexpect(ErrCode::Value::WatUnknownType);
    }
    return TypeUseIdx;
  }
  // The field has only an inline type. Find the type, or make a new one.
  return findOrCreateFuncType(std::move(FuncTy), Mod);
}

// param ::= ( param id valtype ) | ( param valtype* )
// The cursor FC points after the "param" keyword. A form with an id holds
// exactly one valtype, and the id comes first. A form with no id holds a run
// of valtypes. The two forms do not mix, as with (local ...). If Names is not
// null, the function appends the id of each parameter, or an empty string.
Expect<void> Converter::parseParamList(Cursor &FC, std::vector<ValType> &Types,
                                       bool AcceptParamId,
                                       std::vector<std::string> *Names) {
  if (peekType(FC) == NodeType::Id) {
    if (!AcceptParamId) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(auto Id, decodeIdentifier(nodeText(FC.node())));
    FC.next();
    if (!FC.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(auto VT, convertValType(FC.node()));
    FC.next();
    if (FC.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    Types.push_back(VT);
    if (Names) {
      Names->push_back(std::move(Id));
    }
    return {};
  }
  while (FC.valid()) {
    EXPECTED_TRY(auto VT, convertValType(FC.node()));
    Types.push_back(VT);
    if (Names) {
      Names->emplace_back();
    }
    FC.next();
  }
  return {};
}

// limits ::= addrtype? u64 u64? shared?
// addrtype ::= i32 | i64
Expect<AST::Limit> Converter::parseLimits(Cursor &C, bool AllowShared) {
  // The order is fixed. The address type comes first, then one number or two
  // numbers, then the optional shared keyword of a memory.
  bool Is64 = false;
  bool IsShared = false;
  std::vector<uint64_t> Nats;

  if (peekType(C) == NodeType::Keyword) {
    auto CText = nodeText(C.node());
    if (CText == "i64"sv) {
      Is64 = true;
      C.next();
    } else if (CText == "i32"sv) {
      // This is the explicit 32-bit address type, which is the default.
      C.next();
    }
  }
  while (peekType(C) == NodeType::U && Nats.size() < 2) {
    EXPECTED_TRY(auto Val, parseUint(nodeText(C.node())));
    Nats.push_back(Val);
    C.next();
  }
  if (AllowShared && peekType(C) == NodeType::Keyword &&
      nodeText(C.node()) == "shared"sv) {
    IsShared = true;
    C.next();
  }

  // The gates are the gates of the binary loader.
  if (IsShared) {
    EXPECTED_TRY(needProposal(Proposal::Threads,
                              Conf.hasProposal(Proposal::Memory64)
                                  ? ErrCode::Value::MalformedLimitFlags
                                  : ErrCode::Value::IntegerTooLarge));
  }
  if (Is64) {
    EXPECTED_TRY(
        needProposal(Proposal::Memory64, ErrCode::Value::IntegerTooLarge));
  } else if (!Conf.hasProposal(Proposal::Memory64)) {
    for (auto V : Nats) {
      if (V > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max())) {
        return Unexpect(ErrCode::Value::WatI32ConstOutOfRange);
      }
    }
  }
  if (Nats.size() == 2) {
    return AST::Limit(Nats[0], Nats[1], Is64, IsShared);
  } else if (Nats.size() == 1) {
    if (IsShared) {
      AST::Limit Lim(Nats[0], Is64);
      Lim.setType(Is64 ? AST::Limit::LimitType::I64SharedNoMax
                       : AST::Limit::LimitType::SharedNoMax);
      return Lim;
    }
    return AST::Limit(Nats[0], Is64);
  } else {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
}

// tabletype ::= limits reftype
Expect<AST::TableType> Converter::parseTabletype(Cursor &C) {
  // addr_type? limits reftype. A table is never shared.
  EXPECTED_TRY(auto Lim, parseLimits(C, false));
  ValType RefTy(TypeCode::FuncRef);
  if (!C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  EXPECTED_TRY(RefTy, convertRefType(C.node()));
  C.next();
  return AST::TableType(RefTy, Lim);
}

// globaltype ::= valtype | ( mut valtype )
Expect<AST::GlobalType> Converter::parseGlobaltype(Cursor &C) {
  // This is a plain valtype keyword or a (mut valtype) sexpr.
  if (!C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  auto GChild = C.node();
  C.next();
  if (nodeType(GChild) == NodeType::Sexpr) {
    Cursor MC(GChild);
    if (peekType(MC) == NodeType::Keyword && nodeText(MC.node()) == "mut"sv) {
      MC.next(); // Skip the "mut" keyword.
      if (!MC.valid()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      Node Child = MC.node();
      EXPECTED_TRY(auto VT, convertValType(Child));
      MC.next();
      if (MC.valid()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      return AST::GlobalType(VT, ValMut::Var);
    }
  }
  EXPECTED_TRY(auto VT, convertValType(GChild));
  return AST::GlobalType(VT, ValMut::Const);
}

// inlineimport ::= ... ( import name name ) ...
Expect<AST::ImportDesc> Converter::convertInlineImport(Cursor &C, Cursor &FC) {
  AST::ImportDesc Desc;

  {
    std::string Name;

    if (peekType(FC) != NodeType::String) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(Name, parseString(nodeText(FC.node())));
    Desc.setModuleName(Name);
    FC.next();

    if (peekType(FC) != NodeType::String) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(Name, parseString(nodeText(FC.node())));
    Desc.setExternalName(Name);
    FC.next();
  }

  if (FC.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  C.next();
  return Desc;
}

// inlineexport ::= ... ( export name ) ...
Expect<void> Converter::convertInlineExport(Cursor &C, Cursor &FC,
                                            ExternalType ExtType,
                                            uint32_t Idx) {
  if (peekType(FC) != NodeType::String) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  EXPECTED_TRY(auto Name, parseString(nodeText(FC.node())));
  FC.next();
  if (FC.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  PendingExports.push_back({std::move(Name), ExtType, Idx});
  C.next();
  return {};
}

// import ::= ( import name name importdesc )
// importdesc ::= ( func id? typeuse )
//              | ( table id? tabletype )
//              | ( memory id? memtype )
//              | ( global id? globaltype )
//              | ( tag id? typeuse )
Expect<void> Converter::convertImport(Node N, AST::Module &Mod) {
  AST::ImportDesc Desc;
  Cursor C(N);

  if (peekType(C) == NodeType::Keyword) {
    auto KW = nodeText(C.node());
    C.next();
    if (KW != "import"sv) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
  }

  {
    std::string Name;
    if (peekType(C) != NodeType::String) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(Name, parseString(nodeText(C.node())));
    Desc.setModuleName(Name);
    C.next();

    if (peekType(C) != NodeType::String) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(Name, parseString(nodeText(C.node())));
    Desc.setExternalName(Name);
    C.next();
  }

  auto Child = C.node();
  if (nodeType(Child) != NodeType::Sexpr) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  C.next();
  if (C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  Cursor CC(Child);
  if (peekType(CC) != NodeType::Keyword) {
    return Unexpect(ErrCode::Value::WatUnknownOperator);
  }
  auto KW = nodeText(CC.node());
  CC.next(); // Skip the keyword.
  if (peekType(CC) == NodeType::Id) {
    CC.next(); // Skip the optional id.
  }

  if (KW == "func"sv) {
    EXPECTED_TRY(auto Idx, resolveTypeUse(CC, Mod));
    if (CC.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    Desc.setExternalType(ExternalType::Function);
    Desc.setExternalFuncTypeIdx(Idx);
  } else if (KW == "table"sv) {
    EXPECTED_TRY(auto TT, parseTabletype(CC));
    if (CC.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    Desc.setExternalType(ExternalType::Table);
    Desc.getExternalTableType() = TT;
  } else if (KW == "memory"sv) {
    EXPECTED_TRY(auto Lim, parseLimits(CC));
    if (CC.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    Desc.setExternalType(ExternalType::Memory);
    Desc.getExternalMemoryType() = AST::MemoryType(Lim);
  } else if (KW == "global"sv) {
    EXPECTED_TRY(auto GT, parseGlobaltype(CC));
    if (CC.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(checkMutGlobalImport(GT));
    Desc.setExternalType(ExternalType::Global);
    Desc.getExternalGlobalType() = GT;
  } else if (KW == "tag"sv) {
    EXPECTED_TRY(needProposal(Proposal::ExceptionHandling,
                              ErrCode::Value::MalformedImportKind));
    EXPECTED_TRY(auto Idx, resolveTypeUse(CC, Mod));
    if (CC.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    Desc.setExternalType(ExternalType::Tag);
    Desc.getExternalTagType().setTypeIdx(Idx);
  } else {
    return Unexpect(ErrCode::Value::WatUnknownOperator);
  }

  Mod.getImportSection().getContent().push_back(std::move(Desc));
  return {};
}

// export ::= ( export name exportdesc )
// exportdesc ::= ( func funcidx ) | ( table tableidx )
//              | ( memory memidx ) | ( global globalidx )
Expect<void> Converter::convertExport(Node N, AST::Module &Mod) {
  AST::ExportDesc Desc;

  Cursor C(N);
  if (peekType(C) == NodeType::Keyword) {
    auto KW = nodeText(C.node());
    C.next();
    if (KW != "export"sv) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
  }

  if (peekType(C) != NodeType::String) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  auto RawName = nodeText(C.node());
  C.next();
  EXPECTED_TRY(auto Name, parseString(RawName));
  Desc.setExternalName(Name);

  auto Child = C.node();
  if (nodeType(Child) != NodeType::Sexpr) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  C.next();

  if (C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  Cursor IC(Child);
  if (peekType(IC) != NodeType::Keyword) {
    return Unexpect(ErrCode::Value::WatUnknownOperator);
  }
  auto KW = nodeText(IC.node());
  ExternalType ExtType;
  SymbolTable::IndexSpace Space;
  if (KW == "func"sv) {
    ExtType = ExternalType::Function;
    Space = SymbolTable::IndexSpace::Func;
  } else if (KW == "table"sv) {
    ExtType = ExternalType::Table;
    Space = SymbolTable::IndexSpace::Table;
  } else if (KW == "memory"sv) {
    ExtType = ExternalType::Memory;
    Space = SymbolTable::IndexSpace::Memory;
  } else if (KW == "global"sv) {
    ExtType = ExternalType::Global;
    Space = SymbolTable::IndexSpace::Global;
  } else if (KW == "tag"sv) {
    EXPECTED_TRY(needProposal(Proposal::ExceptionHandling,
                              ErrCode::Value::MalformedImportKind));
    ExtType = ExternalType::Tag;
    Space = SymbolTable::IndexSpace::Tag;
  } else {
    return Unexpect(ErrCode::Value::WatUnknownOperator);
  }
  Desc.setExternalType(ExtType);

  // This is the index. The child is an Id or a number.
  IC.next(); // Skip the keyword.
  Node IdxNode = IC.valid() ? IC.node() : Node{};
  auto IT = nodeType(IdxNode);
  if (IT != NodeType::Id && IT != NodeType::U) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  EXPECTED_TRY(auto Idx, Syms.resolve(Space, nodeText(IdxNode)));
  Desc.setExternalIndex(Idx);

  // Reject the tokens that come after the index, for example
  // (export "a" (func 0 1)).
  IC.next();
  if (IC.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  Mod.getExportSection().getContent().push_back(std::move(Desc));
  return {};
}

// func ::= ( func id? (( export name ))* (( import name name ))?
//          typeuse local* expr )
// local ::= ( local id? valtype )
// expr  ::= instr*  (implicit end)
Expect<void> Converter::convertFunc(Node N, AST::Module &Mod) {
  uint32_t ImportedFuncs = 0;
  for (const auto &Imp : Mod.getImportSection().getContent()) {
    if (Imp.getExternalType() == ExternalType::Function) {
      ImportedFuncs++;
    }
  }
  uint32_t FuncIdx =
      ImportedFuncs +
      static_cast<uint32_t>(Mod.getFunctionSection().getContent().size());

  Cursor C(N);
  if (peekType(C) == NodeType::Keyword) {
    auto KW = nodeText(C.node());
    C.next();
    if (KW != "func"sv) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
  }

  if (peekType(C) == NodeType::Id) {
    C.next();
  }

  // Examine the field for an inline import or an inline export.
  while (peekType(C) == NodeType::Sexpr) {
    Node Child = C.node();
    Cursor FC(Child);
    if (peekType(FC) != NodeType::Keyword) {
      break;
    }
    auto Keyword = nodeText(FC.node());
    FC.next();
    if (Keyword == "import"sv) {
      EXPECTED_TRY(auto Desc, convertInlineImport(C, FC));
      EXPECTED_TRY(auto Idx, resolveTypeUse(C, Mod));
      if (C.valid()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      Desc.setExternalType(ExternalType::Function);
      Desc.setExternalFuncTypeIdx(Idx);
      Mod.getImportSection().getContent().push_back(std::move(Desc));
      return {};
    } else if (Keyword == "export"sv) {
      EXPECTED_TRY(convertInlineExport(C, FC, ExternalType::Function, FuncIdx));
    } else {
      break;
    }
  }

  // This is a regular function definition.
  auto TypeUseC = C.copy();
  EXPECTED_TRY(auto TypeIdx, resolveTypeUse(C, Mod));

  Syms.clearLocals();

  // Register the parameter identifiers as locals. The (param ...) groups come
  // after the optional (type ...), and resolveTypeUse examined their shape.
  while (TypeUseC.valid()) {
    Node Child = TypeUseC.node();
    if (sexprMatch(TypeUseC, "param"sv)) {
      std::vector<ValType> Types;
      std::vector<std::string> Names;
      Cursor PC(Child);
      PC.next(); // Skip the "param" keyword.
      EXPECTED_TRY(parseParamList(PC, Types, true, &Names));
      for (auto &Name : Names) {
        if (!Name.empty() &&
            !Syms.Locals.emplace(std::move(Name), Syms.NextLocal).second) {
          return Unexpect(ErrCode::Value::WatDuplicateLocal);
        }
        Syms.NextLocal++;
      }
    } else if (!sexprMatch(TypeUseC, "type"sv)) {
      break;
    }
    TypeUseC.next();
  }

  // Make sure that NextLocal counts all the parameters of the resolved type.
  // A numeric type index can be out of range. The validator then rejects the
  // module, so the count does not matter.
  {
    auto &Types = Mod.getTypeSection().getContent();
    if (TypeIdx < static_cast<uint32_t>(Types.size())) {
      uint32_t NumParams = static_cast<uint32_t>(Types[TypeIdx]
                                                     .getCompositeType()
                                                     .getFuncType()
                                                     .getParamTypes()
                                                     .size());
      if (Syms.NextLocal < NumParams) {
        Syms.NextLocal = NumParams;
      }
    }
  }

  // Parse the local declarations and build the CodeSegment.
  AST::CodeSegment CodeSeg;

  while (peekType(C) == NodeType::Sexpr) {
    Node Child = C.node();
    if (sexprMatch(C, "local"sv)) {
      // local ::= ( local id valtype ) | ( local valtype* )
      // A named local takes one id and one valtype. You cannot mix the two
      // forms.
      Cursor LC(Child);
      LC.next(); // Skip the "local" keyword.
      if (peekType(LC) == NodeType::Id) {
        EXPECTED_TRY(auto LocalId, decodeIdentifier(nodeText(LC.node())));
        LC.next();
        if (!LC.valid()) {
          return Unexpect(ErrCode::Value::WatUnexpectedEnd);
        }
        EXPECTED_TRY(auto VT, convertValType(LC.node()));
        LC.next();
        if (LC.valid()) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        if (!Syms.Locals.emplace(std::move(LocalId), Syms.NextLocal).second) {
          return Unexpect(ErrCode::Value::WatDuplicateLocal);
        }
        CodeSeg.getLocals().emplace_back(1, VT);
        Syms.NextLocal++;
      } else {
        for (; LC.valid(); LC.next()) {
          EXPECTED_TRY(auto VT, convertValType(LC.node()));
          CodeSeg.getLocals().emplace_back(1, VT);
          Syms.NextLocal++;
        }
      }
    } else {
      break;
    }
    C.next();
  }

  // Parse the instructions of the body.
  EXPECTED_TRY(convertExpression(C, CodeSeg.getExpr()));

  if (C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  Mod.getFunctionSection().getContent().push_back(TypeIdx);
  Mod.getCodeSection().getContent().push_back(std::move(CodeSeg));
  return {};
}

// table ::= ( table id? (( export name ))* (( import name name ))? tabletype )
//        | ( table id? reftype ( elem ... ) )
Expect<void> Converter::convertTable(Node N, AST::Module &Mod) {
  uint32_t TableIdx = 0;
  for (const auto &Imp : Mod.getImportSection().getContent()) {
    if (Imp.getExternalType() == ExternalType::Table) {
      TableIdx++;
    }
  }
  TableIdx += static_cast<uint32_t>(Mod.getTableSection().getContent().size());

  Cursor C(N);
  if (peekType(C) == NodeType::Keyword) {
    auto KW = nodeText(C.node());
    C.next();
    if (KW != "table"sv) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
  }

  if (peekType(C) == NodeType::Id) {
    C.next();
  }

  // Examine the field for an inline import or an inline export.
  while (peekType(C) == NodeType::Sexpr) {
    Node Child = C.node();
    Cursor FC(Child);
    if (peekType(FC) != NodeType::Keyword) {
      break;
    }
    auto Keyword = nodeText(FC.node());
    FC.next();
    if (Keyword == "import"sv) {
      EXPECTED_TRY(auto Desc, convertInlineImport(C, FC));
      EXPECTED_TRY(auto TT, parseTabletype(C));
      if (C.valid()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      Desc.setExternalType(ExternalType::Table);
      Desc.getExternalTableType() = TT;
      Mod.getImportSection().getContent().push_back(std::move(Desc));
      return {};
    } else if (Keyword == "export"sv) {
      EXPECTED_TRY(convertInlineExport(C, FC, ExternalType::Table, TableIdx));
    } else {
      break;
    }
  }

  // ( table addr_type? limits reftype expr? )
  // ( table addr_type? reftype ( elem ... ) )

  // Read the addr_type but do not consume it, so that parseTabletype can also
  // see it.
  bool Is64 = false;
  bool HasAddrType = false;
  if (peekType(C) == NodeType::Keyword) {
    auto Text = nodeText(C.node());
    if (Text == "i64"sv) {
      Is64 = true;
      HasAddrType = true;
    } else if (Text == "i32"sv) {
      HasAddrType = true;
    }
  }

  // If a number comes after the optional addr_type, take the limits path.
  {
    auto Ahead = C.copy();
    if (HasAddrType) {
      Ahead.next();
    }
    if (peekType(Ahead) == NodeType::U) {
      AST::TableSegment Seg;
      // parseTabletype calls parseLimits, and parseLimits consumes the
      // addr_type.
      EXPECTED_TRY(auto TT, parseTabletype(C));
      Seg.getTableType() = TT;

      // The optional init expression comes after the table type. It can be
      // a flat instruction sequence or a folded instruction. The binary
      // loader accepts a table init expression only with the Typed Function
      // References proposal.
      if (C.valid()) {
        EXPECTED_TRY(needProposal(Proposal::FunctionReferences,
                                  ErrCode::Value::MalformedTable));
        EXPECTED_TRY(convertExpression(C, Seg.getExpr()));
      }
      if (C.valid()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      Mod.getTableSection().getContent().push_back(std::move(Seg));
    } else {
      // This is an inline elem: addr_type? reftype (elem elem_item*).
      // This path does not call parseLimits, so consume the addr_type here,
      // with the gate of parseLimits.
      if (HasAddrType) {
        if (Is64) {
          EXPECTED_TRY(needProposal(Proposal::Memory64,
                                    ErrCode::Value::IntegerTooLarge));
        }
        C.next();
      }
      ValType RefTy(TypeCode::FuncRef);
      {
        if (!C.valid()) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        EXPECTED_TRY(RefTy, convertRefType(C.node()));
        C.next();
      }

      std::vector<AST::Expression> InitExprs;
      {
        if (!C.valid()) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        if (!sexprMatch(C, "elem"sv)) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        Node EChild = C.node();
        // The form is (elem funcidx*) or (elem elemexpr*). An elemexpr is
        // (item expr) or one folded instruction. The two kinds do not mix.
        bool SeenIdx = false;
        bool SeenExpr = false;
        for (Cursor EC(EChild); EC.next();) {
          Node EEChild = EC.node();
          auto EType = nodeType(EEChild);
          AST::Expression Expr;
          auto &Instrs = Expr.getInstrs();
          if (EType == NodeType::Id || EType == NodeType::U) {
            if (SeenExpr) {
              return Unexpect(ErrCode::Value::WatUnexpectedToken);
            }
            SeenIdx = true;
            EXPECTED_TRY(auto Idx, Syms.resolve(SymbolTable::IndexSpace::Func,
                                                nodeText(EEChild)));
            Instrs.emplace_back(OpCode::Ref__func);
            Instrs.back().getTargetIndex() = Idx;
            Instrs.emplace_back(OpCode::End);
            Instrs.back().setExprLast(true);
          } else if (EType == NodeType::Sexpr) {
            if (SeenIdx) {
              return Unexpect(ErrCode::Value::WatUnexpectedToken);
            }
            SeenExpr = true;
            if (sexprMatch(EC, "item"sv)) {
              Cursor IC(EEChild);
              IC.next(); // Skip the "item" keyword.
              EXPECTED_TRY(convertExpression(IC, Expr));
            } else {
              EXPECTED_TRY(convertFoldedInstr(EEChild, Instrs));
              Instrs.emplace_back(OpCode::End);
              Instrs.back().setExprLast(true);
            }
          } else {
            return Unexpect(ErrCode::Value::WatUnexpectedToken);
          }
          InitExprs.push_back(std::move(Expr));
        }
        C.next();
      }

      uint32_t ElemCount = static_cast<uint32_t>(InitExprs.size());

      AST::TableSegment TSeg;
      AST::Limit Lim(ElemCount, ElemCount, Is64);
      TSeg.getTableType() = AST::TableType(RefTy, Lim);
      Mod.getTableSection().getContent().push_back(std::move(TSeg));

      AST::ElementSegment ESeg;
      ESeg.setMode(AST::ElementSegment::ElemMode::Active);
      ESeg.setRefType(RefTy);
      ESeg.setIdx(TableIdx);
      if (Is64) {
        ESeg.getExpr().getInstrs().emplace_back(OpCode::I64__const);
        ESeg.getExpr().getInstrs().back().setNum(
            static_cast<uint128_t>(static_cast<uint64_t>(0)));
      } else {
        ESeg.getExpr().getInstrs().emplace_back(OpCode::I32__const);
        ESeg.getExpr().getInstrs().back().setNum(
            static_cast<uint128_t>(static_cast<uint32_t>(0)));
      }
      ESeg.getExpr().getInstrs().emplace_back(OpCode::End);
      ESeg.getExpr().getInstrs().back().setExprLast(true);
      ESeg.getInitExprs() = std::move(InitExprs);
      Mod.getElementSection().getContent().push_back(std::move(ESeg));
    }
  }

  if (C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  return {};
}

// memory ::= ( memory id? (( export name ))* (( import name name ))? memtype )
//         | ( memory id? ( data datastring ) )
Expect<void> Converter::convertMemory(Node N, AST::Module &Mod) {
  uint32_t MemIdx = 0;
  for (const auto &Imp : Mod.getImportSection().getContent()) {
    if (Imp.getExternalType() == ExternalType::Memory) {
      MemIdx++;
    }
  }
  MemIdx += static_cast<uint32_t>(Mod.getMemorySection().getContent().size());

  Cursor C(N);
  if (peekType(C) == NodeType::Keyword) {
    auto KW = nodeText(C.node());
    C.next();
    if (KW != "memory"sv) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
  }

  if (peekType(C) == NodeType::Id) {
    C.next();
  }

  // Examine the field for an inline import or an inline export.
  while (peekType(C) == NodeType::Sexpr) {
    Node Child = C.node();
    Cursor FC(Child);
    if (peekType(FC) != NodeType::Keyword) {
      break;
    }
    auto Keyword = nodeText(FC.node());
    FC.next();
    if (Keyword == "import"sv) {
      EXPECTED_TRY(auto Desc, convertInlineImport(C, FC));
      EXPECTED_TRY(auto Lim, parseLimits(C));
      if (C.valid()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      Desc.setExternalType(ExternalType::Memory);
      Desc.getExternalMemoryType() = AST::MemoryType(Lim);
      Mod.getImportSection().getContent().push_back(std::move(Desc));
      return {};
    } else if (Keyword == "export"sv) {
      EXPECTED_TRY(convertInlineExport(C, FC, ExternalType::Memory, MemIdx));
    } else {
      break;
    }
  }

  // If a number comes next, the field holds limits. An i32 or i64 addr_type
  // can come before the number.
  bool IsLimits = false;
  bool Is64 = false;
  {
    auto PeekT = peekType(C);
    if (PeekT == NodeType::U) {
      IsLimits = true;
    } else if (PeekT == NodeType::Keyword) {
      auto Text = nodeText(C.node());
      if (Text == "i64"sv || Text == "i32"sv) {
        // The field holds limits only if a number comes after the keyword.
        auto Ahead = C.copy();
        Ahead.next();
        auto NextT = peekType(Ahead);
        if (NextT == NodeType::U) {
          IsLimits = true;
          Is64 = (Text == "i64"sv);
        } else {
          // A bare i64 or i32 comes before the inline data. Consume it, with
          // the gate of parseLimits, and take the data path.
          Is64 = (Text == "i64"sv);
          if (Is64) {
            EXPECTED_TRY(needProposal(Proposal::Memory64,
                                      ErrCode::Value::IntegerTooLarge));
          }
          C.next();
        }
      }
    }
  }

  if (IsLimits) {
    EXPECTED_TRY(auto Lim, parseLimits(C));
    Mod.getMemorySection().getContent().emplace_back(Lim);
  } else {
    // This is the inline data abbreviation: (memory id? (data b*)). The field
    // holds exactly one (data ...) group and nothing else, so that (memory),
    // (memory bogus), and (memory "abc") do not make a memory.
    std::vector<Byte> DataBytes;
    if (!C.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedEnd);
    }
    if (!sexprMatch(C, "data"sv)) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    for (Cursor DC(C.node()); DC.next();) {
      Node DChild = DC.node();
      if (nodeType(DChild) != NodeType::String) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      EXPECTED_TRY(auto Str, parseString(nodeText(DChild), false));
      DataBytes.insert(DataBytes.end(),
                       reinterpret_cast<const Byte *>(Str.data()),
                       reinterpret_cast<const Byte *>(Str.data() + Str.size()));
    }
    C.next();

    const uint64_t PageSize = 65536;
    uint64_t DataSize = DataBytes.size();
    uint64_t Pages = (DataSize + PageSize - 1) / PageSize;

    Mod.getMemorySection().getContent().emplace_back(
        AST::Limit(Pages, Pages, Is64));

    AST::DataSegment DSeg;
    DSeg.setMode(AST::DataSegment::DataMode::Active);
    DSeg.setIdx(MemIdx);
    if (Is64) {
      DSeg.getExpr().getInstrs().emplace_back(OpCode::I64__const);
      DSeg.getExpr().getInstrs().back().setNum(
          static_cast<uint128_t>(static_cast<uint64_t>(0)));
    } else {
      DSeg.getExpr().getInstrs().emplace_back(OpCode::I32__const);
      DSeg.getExpr().getInstrs().back().setNum(
          static_cast<uint128_t>(static_cast<uint32_t>(0)));
    }
    DSeg.getExpr().getInstrs().emplace_back(OpCode::End);
    DSeg.getExpr().getInstrs().back().setExprLast(true);
    DSeg.getData() = std::move(DataBytes);
    Mod.getDataSection().getContent().push_back(std::move(DSeg));
  }

  if (C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  return {};
}

// global ::= ( global id? (( export name ))* (( import name name ))?
//             globaltype expr )
Expect<void> Converter::convertGlobal(Node N, AST::Module &Mod) {
  uint32_t GlobalIdx = 0;
  for (const auto &Imp : Mod.getImportSection().getContent()) {
    if (Imp.getExternalType() == ExternalType::Global) {
      GlobalIdx++;
    }
  }
  GlobalIdx +=
      static_cast<uint32_t>(Mod.getGlobalSection().getContent().size());

  Cursor C(N);
  if (peekType(C) == NodeType::Keyword) {
    auto KW = nodeText(C.node());
    C.next();
    if (KW != "global"sv) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
  }

  if (peekType(C) == NodeType::Id) {
    C.next();
  }

  // Examine the field for an inline import or an inline export.
  while (peekType(C) == NodeType::Sexpr) {
    Node Child = C.node();
    Cursor FC(Child);
    if (peekType(FC) != NodeType::Keyword) {
      break;
    }
    auto Keyword = nodeText(FC.node());
    FC.next();
    if (Keyword == "import"sv) {
      EXPECTED_TRY(auto Desc, convertInlineImport(C, FC));
      EXPECTED_TRY(auto GT, parseGlobaltype(C));
      if (C.valid()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      EXPECTED_TRY(checkMutGlobalImport(GT));
      Desc.setExternalType(ExternalType::Global);
      Desc.getExternalGlobalType() = GT;
      Mod.getImportSection().getContent().push_back(std::move(Desc));
      return {};
    } else if (Keyword == "export"sv) {
      EXPECTED_TRY(convertInlineExport(C, FC, ExternalType::Global, GlobalIdx));
    } else {
      break;
    }
  }

  AST::GlobalSegment Seg;
  if (!C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  EXPECTED_TRY(auto GT, parseGlobaltype(C));
  Seg.getGlobalType() = GT;
  EXPECTED_TRY(convertExpression(C, Seg.getExpr()));
  Mod.getGlobalSection().getContent().push_back(std::move(Seg));

  if (C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  return {};
}

// start ::= ( start funcidx )
Expect<void> Converter::convertStart(Node N, AST::Module &Mod) {
  Cursor C(N);
  if (peekType(C) == NodeType::Keyword) {
    auto KW = nodeText(C.node());
    C.next();
    if (KW != "start"sv) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
  }
  if (auto Type = peekType(C); Type != NodeType::Id && Type != NodeType::U) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  EXPECTED_TRY(auto Idx,
               Syms.resolve(SymbolTable::IndexSpace::Func, nodeText(C.node())));
  Mod.getStartSection().setContent(Idx);
  C.next();

  if (C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  return {};
}

// The offset of an active segment. The form is (offset expr) or one folded
// instruction. The cursor C points at the sexpr, and the function consumes it.
Expect<void> Converter::convertOffset(Cursor &C, AST::Expression &Expr) {
  if (peekType(C) != NodeType::Sexpr) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  Node OChild = C.node();
  if (sexprMatch(C, "offset"sv)) {
    Cursor OC(OChild);
    OC.next(); // Skip the "offset" keyword.
    C.next();
    EXPECTED_TRY(convertExpression(OC, Expr));
    if (OC.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    return {};
  }
  C.next();
  // This is the abbreviation with one folded instruction. The instruction can
  // hold operands, so the whole sexpr goes to the folded converter.
  auto &Instrs = Expr.getInstrs();
  EXPECTED_TRY(convertFoldedInstr(OChild, Instrs));
  Instrs.emplace_back(OpCode::End);
  Instrs.back().setExprLast(true);
  return {};
}

// Return true if the segment ids exist. Before the Bulk Memory and Reference
// Types proposals, a segment has no id. The id after elem or data is then the
// table or the memory.
bool Converter::hasSegmentIds() const {
  return Conf.hasProposal(Proposal::BulkMemoryOperations) ||
         Conf.hasProposal(Proposal::ReferenceTypes);
}

// elem ::= ( elem id? elemlist )                              passive
//        | ( elem id? tableuse ( offset expr ) elemlist )     active
//        | ( elem id? declare elemlist )                      declarative
// tableuse ::= ( table tableidx )
// elemlist ::= reftype elemexpr* | func funcidx*
// elemexpr ::= ( item expr ) | ( instr )
// The abbreviations are:
// - The tableuse can be a bare numeric index.
// - The tableuse can be absent for table 0.
// - One folded instruction can stand for the offset.
// - An active segment can hold bare funcidx* with no elemlist prefix.
// Before the Bulk Memory and Reference Types proposals, the bare id after
// elem is the table, because a segment has no id.
Expect<void> Converter::convertElem(Node N, AST::Module &Mod) {
  AST::ElementSegment Seg;
  Seg.setMode(AST::ElementSegment::ElemMode::Passive);
  Seg.setRefType(ValType(TypeCode::Ref, TypeCode::FuncRef));
  // A form that the legacy binary encoding cannot hold needs the proposals.
  const auto NeedProposal = [this]() -> Expect<void> {
    if (!hasSegmentIds()) {
      spdlog::error(ErrInfo::InfoProposal(Proposal::BulkMemoryOperations));
      return Unexpect(ErrCode::Value::ExpectedZeroByte);
    }
    return {};
  };

  Cursor C(N);
  if (peekType(C) == NodeType::Keyword) {
    auto KW = nodeText(C.node());
    C.next();
    if (KW != "elem"sv) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
  }

  uint32_t TableIdx = 0;
  bool HasTableUse = false;
  // The elemlist prefix is optional only in the active abbreviation, which
  // has no explicit (table ...) sexpr.
  bool HasTableSexpr = false;
  if (peekType(C) == NodeType::Id) {
    if (hasSegmentIds()) {
      // This is the id of the segment. Pass 1 registered it.
      C.next();
    } else {
      EXPECTED_TRY(TableIdx, Syms.resolve(SymbolTable::IndexSpace::Table,
                                          nodeText(C.node())));
      HasTableUse = true;
      C.next();
    }
  }
  if (peekType(C) == NodeType::U) {
    // This is the legacy bare table index.
    EXPECTED_TRY(TableIdx, Syms.resolve(SymbolTable::IndexSpace::Table,
                                        nodeText(C.node())));
    HasTableUse = true;
    C.next();
  }

  if (peekType(C) == NodeType::Keyword && nodeText(C.node()) == "declare"sv) {
    if (HasTableUse) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(NeedProposal());
    Seg.setMode(AST::ElementSegment::ElemMode::Declarative);
    C.next();
  } else {
    if (!HasTableUse && sexprMatch(C, "table"sv)) {
      HasTableSexpr = true;
      Cursor TC(C.node());
      TC.next(); // Skip the "table" keyword.
      EXPECTED_TRY(Syms.isIndexOrId(TC.node()));
      EXPECTED_TRY(TableIdx, Syms.resolve(SymbolTable::IndexSpace::Table,
                                          nodeText(TC.node())));
      TC.next();
      if (TC.valid()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      HasTableUse = true;
      C.next();
    }
    // A table use needs an offset. Without a table use, an offset is an
    // (offset ...) sexpr or a folded instruction. A (ref ...) sexpr and an
    // (item ...) sexpr start the elemlist instead.
    if (HasTableUse || (peekType(C) == NodeType::Sexpr &&
                        !sexprMatch(C, "ref"sv) && !sexprMatch(C, "item"sv))) {
      EXPECTED_TRY(convertOffset(C, Seg.getExpr()));
      Seg.setMode(AST::ElementSegment::ElemMode::Active);
      Seg.setIdx(TableIdx);
      if (TableIdx != 0) {
        EXPECTED_TRY(NeedProposal());
      }
    } else {
      EXPECTED_TRY(NeedProposal());
    }
  }

  // elemlist ::= reftype elemexpr* | func funcidx*
  // The active abbreviation has no prefix and holds funcidx*. Every other
  // form needs the prefix, even when the list is empty.
  bool SeenRefType = false;
  bool SeenPrefix = false;
  if (peekType(C) == NodeType::Keyword && nodeText(C.node()) == "func"sv) {
    SeenPrefix = true;
    C.next();
  } else if (peekType(C) == NodeType::Keyword || sexprMatch(C, "ref"sv)) {
    EXPECTED_TRY(auto RefTy, convertRefType(C.node()));
    EXPECTED_TRY(NeedProposal());
    Seg.setRefType(RefTy);
    SeenRefType = true;
    SeenPrefix = true;
    C.next();
  }
  if (!SeenPrefix && (HasTableSexpr ||
                      Seg.getMode() != AST::ElementSegment::ElemMode::Active)) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  while (C.valid()) {
    Node Child = C.node();
    AST::Expression Expr;
    auto &Instrs = Expr.getInstrs();
    if (auto Type = nodeType(Child); Type == NodeType::Sexpr) {
      // An elemexpr needs a reftype prefix.
      if (!SeenRefType) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      if (sexprMatch(C, "item"sv)) {
        Cursor IC(Child);
        IC.next(); // Skip the "item" keyword.
        EXPECTED_TRY(convertExpression(IC, Expr));
      } else {
        EXPECTED_TRY(convertFoldedInstr(Child, Instrs));
        Instrs.emplace_back(OpCode::End);
        Instrs.back().setExprLast(true);
      }
    } else if (Type == NodeType::Id || Type == NodeType::U) {
      // A funcidx needs the func prefix or the legacy form with no prefix.
      if (SeenRefType) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      EXPECTED_TRY(auto Idx, Syms.resolve(SymbolTable::IndexSpace::Func,
                                          nodeText(Child)));
      Instrs.emplace_back(OpCode::Ref__func);
      Instrs.back().getTargetIndex() = Idx;
      Instrs.emplace_back(OpCode::End);
      Instrs.back().setExprLast(true);
    } else {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    Seg.getInitExprs().push_back(std::move(Expr));
    C.next();
  }

  Mod.getElementSection().getContent().push_back(std::move(Seg));
  return {};
}

// data ::= ( data id? datastring )                              passive
//        | ( data id? memuse ( offset expr ) datastring )       active
// memuse ::= ( memory memidx )
// datastring ::= string*
// The abbreviations are:
// - The memuse can be a bare numeric index.
// - The memuse can be absent for memory 0.
// - One folded instruction can stand for the offset.
// Before the Bulk Memory and Reference Types proposals, the bare id
// after data is the memory, because a segment has no id.
Expect<void> Converter::convertData(Node N, AST::Module &Mod) {
  AST::DataSegment Seg;
  Seg.setMode(AST::DataSegment::DataMode::Passive);
  const auto NeedProposal = [this]() -> Expect<void> {
    if (!hasSegmentIds()) {
      spdlog::error(ErrInfo::InfoProposal(Proposal::BulkMemoryOperations));
      return Unexpect(ErrCode::Value::ExpectedZeroByte);
    }
    return {};
  };

  Cursor C(N);
  if (peekType(C) == NodeType::Keyword) {
    auto KW = nodeText(C.node());
    C.next();
    if (KW != "data"sv) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
  }

  uint32_t MemIdx = 0;
  bool HasMemUse = false;
  if (peekType(C) == NodeType::Id) {
    if (hasSegmentIds()) {
      // This is the id of the segment. Pass 1 registered it.
      C.next();
    } else {
      EXPECTED_TRY(MemIdx, Syms.resolve(SymbolTable::IndexSpace::Memory,
                                        nodeText(C.node())));
      HasMemUse = true;
      C.next();
    }
  }
  if (peekType(C) == NodeType::U) {
    // This is the legacy bare memory index.
    EXPECTED_TRY(MemIdx, Syms.resolve(SymbolTable::IndexSpace::Memory,
                                      nodeText(C.node())));
    HasMemUse = true;
    C.next();
  }
  if (!HasMemUse && sexprMatch(C, "memory"sv)) {
    Cursor MC(C.node());
    MC.next(); // Skip the "memory" keyword.
    EXPECTED_TRY(Syms.isIndexOrId(MC.node()));
    EXPECTED_TRY(MemIdx, Syms.resolve(SymbolTable::IndexSpace::Memory,
                                      nodeText(MC.node())));
    MC.next();
    if (MC.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    HasMemUse = true;
    C.next();
  }
  // A memory use needs an offset. Without a memory use, a sexpr is an offset
  // too, because a datastring holds only strings.
  if (HasMemUse || peekType(C) == NodeType::Sexpr) {
    EXPECTED_TRY(convertOffset(C, Seg.getExpr()));
    Seg.setMode(AST::DataSegment::DataMode::Active);
    Seg.setIdx(MemIdx);
    if (MemIdx != 0) {
      EXPECTED_TRY(NeedProposal());
    }
  } else {
    EXPECTED_TRY(NeedProposal());
  }

  while (C.valid()) {
    if (peekType(C) != NodeType::String) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    auto RawStr = nodeText(C.node());
    C.next();
    EXPECTED_TRY(auto Str, parseString(RawStr, false));
    auto &Data = Seg.getData();
    Data.insert(Data.end(), reinterpret_cast<const Byte *>(Str.data()),
                reinterpret_cast<const Byte *>(Str.data() + Str.size()));
  }

  Mod.getDataSection().getContent().push_back(std::move(Seg));
  return {};
}

// tag ::= ( tag id? (( export name ))* (( import name name ))? typeuse )
Expect<void> Converter::convertTag(Node N, AST::Module &Mod) {
  // The binary loader rejects the tag section and a tag import before the
  // Exception Handling proposal. The inline import comes below, so the code
  // of the import is the code here too.
  EXPECTED_TRY(needProposal(Proposal::ExceptionHandling,
                            hasInlineImport(N)
                                ? ErrCode::Value::MalformedImportKind
                                : ErrCode::Value::MalformedSection));
  uint32_t TagIdx = 0;
  for (const auto &Imp : Mod.getImportSection().getContent()) {
    if (Imp.getExternalType() == ExternalType::Tag) {
      TagIdx++;
    }
  }
  TagIdx += static_cast<uint32_t>(Mod.getTagSection().getContent().size());

  Cursor C(N);
  if (peekType(C) == NodeType::Keyword) {
    auto KW = nodeText(C.node());
    C.next();
    if (KW != "tag"sv) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
  }
  if (peekType(C) == NodeType::Id) {
    C.next();
  }

  // Examine the field for an inline import or an inline export.
  while (peekType(C) == NodeType::Sexpr) {
    Node Child = C.node();
    Cursor FC(Child);
    if (peekType(FC) != NodeType::Keyword) {
      break;
    }
    auto Keyword = nodeText(FC.node());
    FC.next();
    if (Keyword == "import"sv) {
      EXPECTED_TRY(auto Desc, convertInlineImport(C, FC));
      EXPECTED_TRY(auto Idx, resolveTypeUse(C, Mod));
      if (C.valid()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      Desc.setExternalType(ExternalType::Tag);
      Desc.getExternalTagType().setTypeIdx(Idx);
      Mod.getImportSection().getContent().push_back(std::move(Desc));
      return {};
    } else if (Keyword == "export"sv) {
      EXPECTED_TRY(convertInlineExport(C, FC, ExternalType::Tag, TagIdx));
    } else {
      break;
    }
  }

  // The validator rejects a tag type with a result, as it does for a binary
  // module.
  AST::TagType Tag;
  EXPECTED_TRY(auto Idx, resolveTypeUse(C, Mod));
  if (C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  Tag.setTypeIdx(Idx);
  Mod.getTagSection().getContent().push_back(std::move(Tag));
  return {};
}

} // namespace WAT
} // namespace WasmEdge
