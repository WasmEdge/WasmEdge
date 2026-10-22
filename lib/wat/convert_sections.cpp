// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/errcode.h"
#include "converter.h"
#include "wat/wat_util.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <string>

using namespace std::string_view_literals;

namespace WasmEdge {
namespace WAT {

namespace {

// The size of a page of linear memory.
constexpr uint64_t WasmPageSize = 65536;

// Write the constant offset 0 of an active segment that an inline abbreviation
// makes.
void makeZeroOffset(AST::Expression &Expr, bool Is64) {
  auto &Instrs = Expr.getInstrs();
  if (Is64) {
    Instrs.emplace_back(OpCode::I64__const);
    Instrs.back().setNum(static_cast<uint128_t>(static_cast<uint64_t>(0)));
  } else {
    Instrs.emplace_back(OpCode::I32__const);
    Instrs.back().setNum(static_cast<uint128_t>(static_cast<uint32_t>(0)));
  }
  appendExprEnd(Instrs);
}

} // namespace

// typeuse ::= (type typeidx)? (param valtype*)* (result valtype*)*
// Take the clauses of a typeuse or of a block type at C. A typeuse holds at
// most one (type ...), and it comes first. The (param ...) clauses come before
// the (result ...) clauses. If AcceptParamId is false, a parameter cannot have
// an id.
Expect<Converter::TypeUseClauses>
Converter::parseTypeUseClauses(Cursor &C, bool AcceptParamId) {
  TypeUseClauses Use;
  for (; peekType(C) == NodeType::Sexpr; C.next()) {
    Cursor FC(C.node());
    const auto Keyword = peekKeyword(FC);
    FC.next();
    if (Keyword == "type"sv) {
      if (Use.TypeIdx || Use.hasInline()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      // This is the type index. The child is an Id or a number, and it is the
      // only child.
      EXPECTED_TRY(Use.TypeIdx, takeTypeIdx(FC));
      EXPECTED_TRY(expectEnd(FC));
    } else if (Keyword == "param"sv) {
      if (Use.HasResult) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      Use.HasParam = true;
      EXPECTED_TRY(parseParamList(FC, Use.Inline.getParamTypes(), AcceptParamId,
                                  &Use.ParamNames,
                                  ErrCode::Value::WatUnexpectedToken));
    } else if (Keyword == "result"sv) {
      Use.HasResult = true;
      EXPECTED_TRY(parseResultList(FC, Use.Inline.getReturnTypes()));
    } else {
      break;
    }
  }
  return Use;
}

// Resolve an explicit (type $idx), or an inline list of parameters and
// results, into a type index.
Expect<uint32_t> Converter::resolveTypeUse(Cursor &C, AST::Module &Mod,
                                           bool AcceptParamId) {
  EXPECTED_TRY(auto Use, parseTypeUseClauses(C, AcceptParamId));
  return resolveTypeUse(Use, Mod);
}

// Resolve the clauses Use of a typeuse into a type index.
Expect<uint32_t> Converter::resolveTypeUse(TypeUseClauses &Use,
                                           AST::Module &Mod) {
  if (Use.TypeIdx) {
    EXPECTED_TRY(
        checkTypeUseIdx(Mod, *Use.TypeIdx, Use.hasInline(), Use.Inline));
    return *Use.TypeIdx;
  }
  // The field has only an inline type. Find the type, or make a new one.
  return findOrCreateFuncType(std::move(Use.Inline), Mod);
}

// Examine the explicit type index TypeIdx of a typeuse or of a block type.
// HasInline is true if an inline type Inline comes with it.
Expect<void> Converter::checkTypeUseIdx(const AST::Module &Mod,
                                        uint32_t TypeIdx, bool HasInline,
                                        const AST::FunctionType &Inline) const {
  const auto &Types = Mod.getTypeSection().getContent();
  if (TypeIdx < Types.size()) {
    // A typeuse must give a function type. Reject a GC struct type or array
    // type that is already complete. A caller such as convertFunc calls
    // getFuncType(), and it must not dereference a null FunctionType variant.
    if (!Types[TypeIdx].getCompositeType().isFunc()) {
      return Unexpect(ErrCode::Value::WatUnknownType);
    }
    // If an inline type comes with the type index, the two types must be
    // equal. The comparison is structural, because a type with the same
    // structure can come before the named type.
    if (HasInline) {
      const auto &RefTy = Types[TypeIdx].getCompositeType().getFuncType();
      if (Inline.getParamTypes() != RefTy.getParamTypes() ||
          Inline.getReturnTypes() != RefTy.getReturnTypes()) {
        return Unexpect(ErrCode::Value::WatInlineFuncType);
      }
    }
  } else if (HasInline) {
    // An inline type needs the type at this point for the comparison. As a
    // result, a numeric index that is out of range is a syntax error here.
    // Without an inline type, the validator reports the index, as it does for
    // a binary module.
    return Unexpect(ErrCode::Value::WatUnknownType);
  }
  return {};
}

// param ::= ( param id valtype ) | ( param valtype* )
// local ::= ( local id valtype ) | ( local valtype* )
// The cursor FC points after the "param" or "local" keyword. A form with an id
// holds exactly one valtype, and the id comes first. A form with no id holds a
// run of valtypes. The two forms do not mix. If Names is not null, the function
// appends the id of each entry, or an empty string. MissingTypeErr is the error
// code of an id with no valtype.
Expect<void> Converter::parseParamList(Cursor &FC, std::vector<ValType> &Types,
                                       bool AcceptParamId,
                                       std::vector<std::string> *Names,
                                       ErrCode::Value MissingTypeErr) {
  if (peekType(FC) == NodeType::Id) {
    if (!AcceptParamId) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(auto Id, decodeIdentifier(nodeText(FC.node())));
    FC.next();
    if (!FC.valid()) {
      return Unexpect(MissingTypeErr);
    }
    EXPECTED_TRY(auto VT, convertValType(FC.node()));
    FC.next();
    EXPECTED_TRY(expectEnd(FC));
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

// result ::= ( result valtype* )
// The cursor RC points after the "result" keyword. The function appends the
// valtypes up to the end of RC to Types. A result has no id.
Expect<void> Converter::parseResultList(Cursor &RC,
                                        std::vector<ValType> &Types) {
  for (; RC.valid(); RC.next()) {
    EXPECTED_TRY(auto VT, convertValType(RC.node()));
    Types.push_back(VT);
  }
  return {};
}

// addrtype ::= i32 | i64
// If C is at an address type, give true for i64 and false for i32. Otherwise,
// give no value. C does not move.
std::optional<bool> Converter::peekAddrType(const Cursor &C) const {
  const auto KW = peekKeyword(C);
  if (KW == "i64"sv) {
    return true;
  }
  if (KW == "i32"sv) {
    return false;
  }
  return std::nullopt;
}

// Return true if a number comes after the optional address type AddrType at C.
// The field then holds limits and not an inline abbreviation.
bool Converter::atLimits(const Cursor &C, std::optional<bool> AddrType) const {
  auto Ahead = C.copy();
  if (AddrType) {
    Ahead.next();
  }
  return peekType(Ahead) == NodeType::U;
}

// Take the address type of an inline abbreviation at C. An abbreviation does
// not call parseLimits, so the function applies the gate of parseLimits.
Expect<void> Converter::takeAbbrevAddrType(Cursor &C, bool Is64) {
  if (Is64) {
    EXPECTED_TRY(
        needProposal(Proposal::Memory64, ErrCode::Value::IntegerTooLarge));
  }
  C.next();
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

  if (const auto AddrType = peekAddrType(C)) {
    Is64 = *AddrType;
    C.next();
  }
  while (peekType(C) == NodeType::U && Nats.size() < 2) {
    EXPECTED_TRY(auto Val, parseUint(nodeText(C.node())));
    Nats.push_back(Val);
    C.next();
  }
  if (AllowShared && peekKeyword(C) == "shared"sv) {
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
  }
  if (Nats.size() == 1) {
    if (IsShared) {
      AST::Limit Lim(Nats[0], Is64);
      Lim.setType(Is64 ? AST::Limit::LimitType::I64SharedNoMax
                       : AST::Limit::LimitType::SharedNoMax);
      return Lim;
    }
    return AST::Limit(Nats[0], Is64);
  }
  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

// tabletype ::= limits reftype
Expect<AST::TableType> Converter::parseTabletype(Cursor &C) {
  // addr_type? limits reftype. A table is never shared.
  EXPECTED_TRY(auto Lim, parseLimits(C, /*AllowShared=*/false));
  if (!C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  EXPECTED_TRY(auto RefTy, convertRefType(C.node()));
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
  if (sexprMatch(GChild, "mut"sv)) {
    EXPECTED_TRY(auto VT,
                 convertMutOperand(GChild, &Converter::convertValType));
    return AST::GlobalType(VT, ValMut::Var);
  }
  EXPECTED_TRY(auto VT, convertValType(GChild));
  return AST::GlobalType(VT, ValMut::Const);
}

// Take the module name and the external name of an import at C into Desc.
Expect<void> Converter::takeImportNames(Cursor &C, AST::ImportDesc &Desc) {
  EXPECTED_TRY(auto ModName, takeName(C));
  Desc.setModuleName(ModName);
  EXPECTED_TRY(auto ExtName, takeName(C));
  Desc.setExternalName(ExtName);
  return {};
}

// The descriptor sexpr of an import or an export at C, such as (func ...). The
// descriptor is the last child of the field. The function gives the kind of
// the descriptor and a cursor after its keyword.
Expect<std::pair<Converter::ExternKind, Cursor>>
Converter::takeExternDesc(Cursor &C) {
  auto Child = C.node();
  if (nodeType(Child) != NodeType::Sexpr) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  C.next();
  EXPECTED_TRY(expectEnd(C));

  Cursor DC(Child);
  auto Kind = externKind(peekKeyword(DC));
  if (!Kind) {
    return Unexpect(ErrCode::Value::WatUnknownOperator);
  }
  DC.next(); // Skip the keyword.
  return std::pair<ExternKind, Cursor>{*Kind, std::move(DC)};
}

// inlineimport ::= ... ( import name name ) ...
Expect<AST::ImportDesc> Converter::convertInlineImport(Cursor &FC) {
  AST::ImportDesc Desc;
  EXPECTED_TRY(takeImportNames(FC, Desc));
  EXPECTED_TRY(expectEnd(FC));
  return Desc;
}

// inlineexport ::= ... ( export name ) ...
Expect<void> Converter::convertInlineExport(Cursor &FC, ExternalType ExtType,
                                            uint32_t Idx, AST::Module &Mod) {
  EXPECTED_TRY(auto Name, takeName(FC));
  EXPECTED_TRY(expectEnd(FC));
  addExport(Name, ExtType, Idx, Mod);
  return {};
}

// Append an export of the name Name for the index Idx of the kind ExtType.
void Converter::addExport(std::string_view Name, ExternalType ExtType,
                          uint32_t Idx, AST::Module &Mod) {
  auto &Desc = Mod.getExportSection().getContent().emplace_back();
  Desc.setExternalName(Name);
  Desc.setExternalType(ExtType);
  Desc.setExternalIndex(Idx);
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
  C.next(); // Skip the "import" keyword.

  EXPECTED_TRY(takeImportNames(C, Desc));
  EXPECTED_TRY(auto KindDesc, takeExternDesc(C));
  auto &[Kind, DC] = KindDesc;
  takeOptionalId(DC);
  EXPECTED_TRY(convertImportDesc(Kind.Type, DC, Desc, Mod));
  addImport(std::move(Desc), Mod);
  return {};
}

// Append the import Desc to the module, and count it for its external type.
void Converter::addImport(AST::ImportDesc Desc, AST::Module &Mod) {
  ++ImportCounts[static_cast<size_t>(Desc.getExternalType())];
  Mod.getImportSection().getContent().push_back(std::move(Desc));
}

// The description of an import of the kind ExtType, after the optional id:
// typeuse, tabletype, memtype, globaltype, or the typeuse of a tag. The
// description is the rest of C.
Expect<void> Converter::convertImportDesc(ExternalType ExtType, Cursor &C,
                                          AST::ImportDesc &Desc,
                                          AST::Module &Mod) {
  Desc.setExternalType(ExtType);
  switch (ExtType) {
  case ExternalType::Function: {
    EXPECTED_TRY(auto Idx, resolveTypeUse(C, Mod));
    EXPECTED_TRY(expectEnd(C));
    Desc.setExternalFuncTypeIdx(Idx);
    return {};
  }
  case ExternalType::Table: {
    EXPECTED_TRY(auto TT, parseTabletype(C));
    EXPECTED_TRY(expectEnd(C));
    Desc.getExternalTableType() = TT;
    return {};
  }
  case ExternalType::Memory: {
    EXPECTED_TRY(auto Lim, parseLimits(C));
    EXPECTED_TRY(expectEnd(C));
    Desc.getExternalMemoryType() = AST::MemoryType(Lim);
    return {};
  }
  case ExternalType::Global: {
    EXPECTED_TRY(auto GT, parseGlobaltype(C));
    EXPECTED_TRY(expectEnd(C));
    EXPECTED_TRY(checkMutGlobalImport(GT));
    Desc.getExternalGlobalType() = GT;
    return {};
  }
  case ExternalType::Tag: {
    EXPECTED_TRY(needProposal(Proposal::ExceptionHandling,
                              ErrCode::Value::MalformedImportKind));
    EXPECTED_TRY(auto Idx, resolveTypeUse(C, Mod));
    EXPECTED_TRY(expectEnd(C));
    Desc.getExternalTagType().setTypeIdx(Idx);
    return {};
  }
  default:
    assumingUnreachable();
  }
}

// Convert the inline (import ...) and (export ...) abbreviations of a func,
// table, memory, global, or tag field of the kind ExtType. C points after the
// optional id, and Idx is the index of the field. If the field has an inline
// import, the function converts the rest of the field as the import
// description and returns true.
Expect<bool> Converter::convertInlineImportExport(Cursor &C,
                                                  ExternalType ExtType,
                                                  uint32_t Idx,
                                                  AST::Module &Mod) {
  while (peekType(C) == NodeType::Sexpr) {
    Cursor FC(C.node());
    if (peekType(FC) != NodeType::Keyword) {
      break;
    }
    auto Keyword = nodeText(FC.node());
    FC.next();
    if (Keyword == "import"sv) {
      EXPECTED_TRY(auto Desc, convertInlineImport(FC));
      C.next();
      EXPECTED_TRY(convertImportDesc(ExtType, C, Desc, Mod));
      addImport(std::move(Desc), Mod);
      return true;
    }
    if (Keyword != "export"sv) {
      break;
    }
    EXPECTED_TRY(convertInlineExport(FC, ExtType, Idx, Mod));
    C.next();
  }
  return false;
}

// Start a func, table, memory, global, or tag field of the kind ExtType. C
// points at the keyword of the field, and DefCount is the count of the
// definitions of the kind before the field. The function skips the keyword and
// the optional id, and converts the inline imports and exports. If the field is
// an inline import, the function returns no index. Otherwise, it returns the
// index of the field, and C points at the rest of the field.
Expect<std::optional<uint32_t>>
Converter::beginImportableField(Cursor &C, ExternalType ExtType,
                                size_t DefCount, AST::Module &Mod) {
  const uint32_t Idx = ImportCounts[static_cast<size_t>(ExtType)] +
                       static_cast<uint32_t>(DefCount);
  C.next(); // Skip the keyword.
  takeOptionalId(C);
  EXPECTED_TRY(auto IsImport, convertInlineImportExport(C, ExtType, Idx, Mod));
  if (IsImport) {
    return std::nullopt;
  }
  return Idx;
}

// export ::= ( export name exportdesc )
// exportdesc ::= ( func funcidx ) | ( table tableidx )
//              | ( memory memidx ) | ( global globalidx )
Expect<void> Converter::convertExport(Node N, AST::Module &Mod) {
  Cursor C(N);
  C.next(); // Skip the "export" keyword.

  EXPECTED_TRY(auto Name, takeName(C));
  EXPECTED_TRY(auto KindDesc, takeExternDesc(C));
  auto &[Kind, DC] = KindDesc;
  if (Kind.Type == ExternalType::Tag) {
    EXPECTED_TRY(needProposal(Proposal::ExceptionHandling,
                              ErrCode::Value::MalformedImportKind));
  }

  // This is the index. The child is an Id or a number.
  EXPECTED_TRY(auto Idx, takeIdx(DC, Kind.Space));

  // Reject the tokens that come after the index, for example
  // (export "a" (func 0 1)).
  EXPECTED_TRY(expectEnd(DC));

  addExport(Name, Kind.Type, Idx, Mod);
  return {};
}

// func ::= ( func id? (( export name ))* (( import name name ))?
//          typeuse local* expr )
// local ::= ( local id? valtype )
// expr  ::= instr*  (implicit end)
Expect<void> Converter::convertFunc(Node N, AST::Module &Mod) {
  Cursor C(N);
  EXPECTED_TRY(
      auto FuncIdx,
      beginImportableField(C, ExternalType::Function,
                           Mod.getFunctionSection().getContent().size(), Mod));
  if (!FuncIdx) {
    return {};
  }

  // This is a regular function definition.
  EXPECTED_TRY(auto Use, parseTypeUseClauses(C, /*AcceptParamId=*/true));
  EXPECTED_TRY(auto TypeIdx, resolveTypeUse(Use, Mod));

  // Register the parameter identifiers as locals.
  Syms.clearLocals();
  for (auto &Name : Use.ParamNames) {
    EXPECTED_TRY(Syms.define(SymbolTable::IndexSpace::Local, std::move(Name),
                             ErrCode::Value::WatDuplicateLocal));
  }

  // Make sure that the next local index counts all the parameters of the
  // resolved type. A numeric type index can be out of range. The validator then
  // rejects the module, so the count does not matter.
  {
    auto &Types = Mod.getTypeSection().getContent();
    if (TypeIdx < static_cast<uint32_t>(Types.size())) {
      uint32_t NumParams = static_cast<uint32_t>(Types[TypeIdx]
                                                     .getCompositeType()
                                                     .getFuncType()
                                                     .getParamTypes()
                                                     .size());
      auto &NextLocal = Syms.next(SymbolTable::IndexSpace::Local);
      NextLocal = std::max(NextLocal, NumParams);
    }
  }

  // Parse the local declarations and build the CodeSegment.
  AST::CodeSegment CodeSeg;

  for (; sexprMatch(C, "local"sv); C.next()) {
    std::vector<ValType> Types;
    std::vector<std::string> Names;
    Cursor LC(C.node());
    LC.next(); // Skip the "local" keyword.
    EXPECTED_TRY(parseParamList(LC, Types, /*AcceptParamId=*/true, &Names,
                                ErrCode::Value::WatUnexpectedEnd));
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(Syms.define(SymbolTable::IndexSpace::Local,
                               std::move(Names[I]),
                               ErrCode::Value::WatDuplicateLocal));
      CodeSeg.getLocals().emplace_back(1, Types[I]);
    }
  }

  // Parse the instructions of the body.
  EXPECTED_TRY(convertExpression(C, CodeSeg.getExpr()));

  Mod.getFunctionSection().getContent().push_back(TypeIdx);
  Mod.getCodeSection().getContent().push_back(std::move(CodeSeg));
  return {};
}

// table ::= ( table id? (( export name ))* (( import name name ))? tabletype )
//        | ( table id? reftype ( elem ... ) )
Expect<void> Converter::convertTable(Node N, AST::Module &Mod) {
  Cursor C(N);
  EXPECTED_TRY(auto TableIdx,
               beginImportableField(C, ExternalType::Table,
                                    Mod.getTableSection().getContent().size(),
                                    Mod));
  if (!TableIdx) {
    return {};
  }

  // ( table addr_type? limits reftype expr? )
  // ( table addr_type? reftype ( elem ... ) )
  // If a number comes after the optional addr_type, take the limits path.
  const auto AddrType = peekAddrType(C);
  const bool Is64 = AddrType.value_or(false);
  if (atLimits(C, AddrType)) {
    AST::TableSegment Seg;
    // parseTabletype calls parseLimits, and parseLimits consumes the
    // addr_type.
    EXPECTED_TRY(auto TT, parseTabletype(C));
    Seg.getTableType() = TT;

    // The optional init expression comes after the table type. It can be a
    // flat instruction sequence or a folded instruction. The binary loader
    // accepts a table init expression only with the Typed Function References
    // proposal.
    if (C.valid()) {
      EXPECTED_TRY(needProposal(Proposal::FunctionReferences,
                                ErrCode::Value::MalformedTable));
      EXPECTED_TRY(convertExpression(C, Seg.getExpr()));
    }
    Mod.getTableSection().getContent().push_back(std::move(Seg));
  } else {
    // This is the inline elem abbreviation.
    if (AddrType) {
      EXPECTED_TRY(takeAbbrevAddrType(C, Is64));
    }
    EXPECTED_TRY(convertTableInlineElem(C, *TableIdx, Is64, Mod));
  }

  EXPECTED_TRY(expectEnd(C));

  return {};
}

// The inline elem abbreviation of a table: addr_type? reftype (elem
// elem_item*). C points at the reftype, after the address type. The function
// makes the table of the index TableIdx and its active element segment.
Expect<void> Converter::convertTableInlineElem(Cursor &C, uint32_t TableIdx,
                                               bool Is64, AST::Module &Mod) {
  if (!C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  EXPECTED_TRY(auto RefTy, convertRefType(C.node()));
  C.next();

  if (!sexprMatch(C, "elem"sv)) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  // The form is (elem funcidx*) or (elem elemexpr*). The first item gives the
  // kind.
  std::vector<AST::Expression> InitExprs;
  Cursor EC(C.node());
  EC.next(); // Skip the "elem" keyword.
  EXPECTED_TRY(appendElemItems(EC, std::nullopt, InitExprs));
  C.next();

  uint32_t ElemCount = static_cast<uint32_t>(InitExprs.size());

  AST::TableSegment TSeg;
  AST::Limit Lim(ElemCount, ElemCount, Is64);
  TSeg.getTableType() = AST::TableType(RefTy, Lim);
  Mod.getTableSection().getContent().push_back(std::move(TSeg));

  AST::ElementSegment ESeg;
  ESeg.setMode(AST::ElementSegment::ElemMode::Active);
  ESeg.setRefType(RefTy);
  ESeg.setIdx(TableIdx);
  makeZeroOffset(ESeg.getExpr(), Is64);
  ESeg.getInitExprs() = std::move(InitExprs);
  Mod.getElementSection().getContent().push_back(std::move(ESeg));
  return {};
}

// memory ::= ( memory id? (( export name ))* (( import name name ))? memtype )
//         | ( memory id? ( data datastring ) )
Expect<void> Converter::convertMemory(Node N, AST::Module &Mod) {
  Cursor C(N);
  EXPECTED_TRY(auto MemIdx,
               beginImportableField(C, ExternalType::Memory,
                                    Mod.getMemorySection().getContent().size(),
                                    Mod));
  if (!MemIdx) {
    return {};
  }

  // If a number comes next, the field holds limits. An i32 or i64 addr_type
  // can come before the number.
  const auto AddrType = peekAddrType(C);
  const bool Is64 = AddrType.value_or(false);
  if (atLimits(C, AddrType)) {
    EXPECTED_TRY(auto Lim, parseLimits(C));
    Mod.getMemorySection().getContent().emplace_back(Lim);
  } else {
    // A bare i64 or i32 can come before the inline data.
    if (AddrType) {
      EXPECTED_TRY(takeAbbrevAddrType(C, Is64));
    }
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
    Cursor DC(C.node());
    DC.next(); // Skip the "data" keyword.
    EXPECTED_TRY(appendDataString(DC, DataBytes));
    C.next();

    const uint64_t DataSize = DataBytes.size();
    const uint64_t Pages = (DataSize + WasmPageSize - 1) / WasmPageSize;

    Mod.getMemorySection().getContent().emplace_back(
        AST::Limit(Pages, Pages, Is64));

    AST::DataSegment DSeg;
    DSeg.setMode(AST::DataSegment::DataMode::Active);
    DSeg.setIdx(*MemIdx);
    makeZeroOffset(DSeg.getExpr(), Is64);
    DSeg.getData() = std::move(DataBytes);
    Mod.getDataSection().getContent().push_back(std::move(DSeg));
  }

  EXPECTED_TRY(expectEnd(C));

  return {};
}

// global ::= ( global id? (( export name ))* (( import name name ))?
//             globaltype expr )
Expect<void> Converter::convertGlobal(Node N, AST::Module &Mod) {
  Cursor C(N);
  EXPECTED_TRY(auto GlobalIdx,
               beginImportableField(C, ExternalType::Global,
                                    Mod.getGlobalSection().getContent().size(),
                                    Mod));
  if (!GlobalIdx) {
    return {};
  }

  AST::GlobalSegment Seg;
  EXPECTED_TRY(auto GT, parseGlobaltype(C));
  Seg.getGlobalType() = GT;
  EXPECTED_TRY(convertExpression(C, Seg.getExpr()));
  Mod.getGlobalSection().getContent().push_back(std::move(Seg));
  return {};
}

// start ::= ( start funcidx )
Expect<void> Converter::convertStart(Node N, AST::Module &Mod) {
  Cursor C(N);
  C.next(); // Skip the "start" keyword.
  EXPECTED_TRY(auto Idx, takeIdx(C, SymbolTable::IndexSpace::Func));
  Mod.getStartSection().setContent(Idx);

  EXPECTED_TRY(expectEnd(C));

  return {};
}

// Convert the sexpr N into Expr. The form of N is (KW expr) or one folded
// instruction.
Expect<void> Converter::convertWrappedExpr(Node N, std::string_view KW,
                                           AST::Expression &Expr) {
  if (sexprMatch(N, KW)) {
    Cursor EC(N);
    EC.next(); // Skip the keyword.
    return convertExpression(EC, Expr);
  }
  // This is the abbreviation with one folded instruction. The instruction can
  // hold operands, so the whole sexpr goes to the folded converter.
  auto &Instrs = Expr.getInstrs();
  EXPECTED_TRY(convertFoldedInstr(N, Instrs));
  appendExprEnd(Instrs);
  return {};
}

// The offset of an active segment. The form is (offset expr) or one folded
// instruction. The cursor C points at the sexpr, and the function consumes it.
Expect<void> Converter::convertOffset(Cursor &C, AST::Expression &Expr) {
  if (peekType(C) != NodeType::Sexpr) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  Node OChild = C.node();
  C.next();
  return convertWrappedExpr(OChild, "offset"sv, Expr);
}

// One item of an element list. N is a funcidx, an (item expr) sexpr, or one
// folded instruction. The caller examines the kind of N.
Expect<AST::Expression> Converter::convertElemItem(Node N) {
  AST::Expression Expr;
  auto &Instrs = Expr.getInstrs();
  if (nodeType(N) == NodeType::Sexpr) {
    EXPECTED_TRY(convertWrappedExpr(N, "item"sv, Expr));
    return Expr;
  }
  EXPECTED_TRY(auto Idx,
               Syms.resolve(SymbolTable::IndexSpace::Func, nodeText(N)));
  Instrs.emplace_back(OpCode::Ref__func);
  Instrs.back().getTargetIndex() = Idx;
  appendExprEnd(Instrs);
  return Expr;
}

// Append the element items at C to Out, up to the end of C. An item is a
// funcidx or an elemexpr, and the two kinds do not mix. IsExpr gives the kind.
// If IsExpr has no value, the first item gives the kind.
Expect<void> Converter::appendElemItems(Cursor &C, std::optional<bool> IsExpr,
                                        std::vector<AST::Expression> &Out) {
  for (; C.valid(); C.next()) {
    Node Child = C.node();
    bool ItemIsExpr = false;
    if (nodeType(Child) == NodeType::Sexpr) {
      ItemIsExpr = true;
    } else if (!isIndexOrId(Child)) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    if (!IsExpr) {
      IsExpr = ItemIsExpr;
    } else if (*IsExpr != ItemIsExpr) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(auto Expr, convertElemItem(Child));
    Out.push_back(std::move(Expr));
  }
  return {};
}

// Return true if the segment ids exist. Before the Bulk Memory and Reference
// Types proposals, a segment has no id. The id after elem or data is then the
// table or the memory.
bool Converter::hasSegmentIds() const {
  return Conf.hasProposal(Proposal::BulkMemoryOperations) ||
         Conf.hasProposal(Proposal::ReferenceTypes);
}

// Make sure that the segment ids exist. A segment form that the legacy binary
// encoding cannot hold needs the proposals.
Expect<void> Converter::needSegmentIds() const {
  if (hasSegmentIds()) {
    return {};
  }
  return needProposal(Proposal::BulkMemoryOperations,
                      ErrCode::Value::ExpectedZeroByte);
}

// The target of an elem or a data segment before the (table ...) or the
// (memory ...) sexpr. Before the Bulk Memory and Reference Types proposals,
// the bare id is the target, because a segment has no id. A bare number is the
// legacy target index. The function returns true if it takes a target into
// Idx.
Expect<bool> Converter::takeLegacySegmentTarget(Cursor &C,
                                                SymbolTable::IndexSpace Space,
                                                uint32_t &Idx) {
  bool HasTarget = false;
  if (peekType(C) == NodeType::Id) {
    if (hasSegmentIds()) {
      // This is the id of the segment. Pass 1 registered it.
      C.next();
    } else {
      EXPECTED_TRY(Idx, takeIdx(C, Space));
      HasTarget = true;
    }
  }
  if (peekType(C) == NodeType::U) {
    EXPECTED_TRY(Idx, takeIdx(C, Space));
    HasTarget = true;
  }
  return HasTarget;
}

// The (table tableidx) or (memory memidx) target of a segment. If C is at a
// sexpr that starts with KW, the function takes its index into Idx, moves C
// past it, and returns true.
Expect<bool> Converter::takeSegmentUse(Cursor &C, std::string_view KW,
                                       SymbolTable::IndexSpace Space,
                                       uint32_t &Idx) {
  if (!sexprMatch(C, KW)) {
    return false;
  }
  Cursor UC(C.node());
  UC.next(); // Skip the keyword.
  EXPECTED_TRY(Idx, takeIdx(UC, Space));
  EXPECTED_TRY(expectEnd(UC));
  C.next();
  return true;
}

// datastring ::= string*
// Append the bytes of the strings at C to Out, up to the end of C.
Expect<void> Converter::appendDataString(Cursor &C, std::vector<Byte> &Out) {
  for (; C.valid(); C.next()) {
    if (peekType(C) != NodeType::String) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(auto Str,
                 parseString(nodeText(C.node()), /*ValidateUTF8=*/false));
    Out.insert(Out.end(), reinterpret_cast<const Byte *>(Str.data()),
               reinterpret_cast<const Byte *>(Str.data() + Str.size()));
  }
  return {};
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

  Cursor C(N);
  C.next(); // Skip the "elem" keyword.

  uint32_t TableIdx = 0;
  EXPECTED_TRY(
      bool HasTableUse,
      takeLegacySegmentTarget(C, SymbolTable::IndexSpace::Table, TableIdx));
  // The elemlist prefix is optional only in the active abbreviation, which
  // has no explicit (table ...) sexpr.
  bool HasTableSexpr = false;

  if (peekKeyword(C) == "declare"sv) {
    if (HasTableUse) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(needSegmentIds());
    Seg.setMode(AST::ElementSegment::ElemMode::Declarative);
    C.next();
  } else {
    if (!HasTableUse) {
      EXPECTED_TRY(HasTableSexpr,
                   takeSegmentUse(C, "table"sv, SymbolTable::IndexSpace::Table,
                                  TableIdx));
      HasTableUse = HasTableSexpr;
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
        EXPECTED_TRY(needSegmentIds());
      }
    } else {
      EXPECTED_TRY(needSegmentIds());
    }
  }

  // elemlist ::= reftype elemexpr* | func funcidx*
  // The active abbreviation has no prefix and holds funcidx*. Every other
  // form needs the prefix, even when the list is empty.
  bool SeenRefType = false;
  bool SeenPrefix = false;
  if (peekKeyword(C) == "func"sv) {
    SeenPrefix = true;
    C.next();
  } else if (peekType(C) == NodeType::Keyword || sexprMatch(C, "ref"sv)) {
    EXPECTED_TRY(auto RefTy, convertRefType(C.node()));
    EXPECTED_TRY(needSegmentIds());
    Seg.setRefType(RefTy);
    SeenRefType = true;
    SeenPrefix = true;
    C.next();
  }
  if (!SeenPrefix && (HasTableSexpr ||
                      Seg.getMode() != AST::ElementSegment::ElemMode::Active)) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  // An elemexpr needs a reftype prefix. A funcidx needs the func prefix or the
  // legacy form with no prefix.
  EXPECTED_TRY(appendElemItems(C, SeenRefType, Seg.getInitExprs()));

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

  Cursor C(N);
  C.next(); // Skip the "data" keyword.

  uint32_t MemIdx = 0;
  EXPECTED_TRY(bool HasMemUse, takeLegacySegmentTarget(
                                   C, SymbolTable::IndexSpace::Memory, MemIdx));
  if (!HasMemUse) {
    EXPECTED_TRY(
        HasMemUse,
        takeSegmentUse(C, "memory"sv, SymbolTable::IndexSpace::Memory, MemIdx));
  }
  // A memory use needs an offset. Without a memory use, a sexpr is an offset
  // too, because a datastring holds only strings.
  if (HasMemUse || peekType(C) == NodeType::Sexpr) {
    EXPECTED_TRY(convertOffset(C, Seg.getExpr()));
    Seg.setMode(AST::DataSegment::DataMode::Active);
    Seg.setIdx(MemIdx);
    if (MemIdx != 0) {
      EXPECTED_TRY(needSegmentIds());
    }
  } else {
    EXPECTED_TRY(needSegmentIds());
  }

  EXPECTED_TRY(appendDataString(C, Seg.getData()));

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
  Cursor C(N);
  EXPECTED_TRY(auto TagIdx, beginImportableField(
                                C, ExternalType::Tag,
                                Mod.getTagSection().getContent().size(), Mod));
  if (!TagIdx) {
    return {};
  }

  // The validator rejects a tag type with a result, as it does for a binary
  // module.
  AST::TagType Tag;
  EXPECTED_TRY(auto Idx, resolveTypeUse(C, Mod));
  EXPECTED_TRY(expectEnd(C));
  Tag.setTypeIdx(Idx);
  Mod.getTagSection().getContent().push_back(std::move(Tag));
  return {};
}

} // namespace WAT
} // namespace WasmEdge
