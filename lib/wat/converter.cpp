// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "converter.h"
#include "common/errcode.h"
#include "common/errinfo.h"
#include "common/spdlog.h"
#include "wat/lexical.h"
#include "wat/wat_util.h"

#include <cctype>
#include <optional>
#include <utility>
#include <vector>

using namespace std::string_view_literals;

namespace WasmEdge {
namespace WAT {

namespace {

// Decode the id Ref, and find it in Map. A malformed id and a missing id both
// give the error code Err.
template <typename MapT>
Expect<const typename MapT::mapped_type *>
lookupId(const MapT &Map, std::string_view Ref, ErrCode::Value Err) {
  EXPECTED_TRY(auto Decoded,
               decodeIdentifier(Ref).map_error([Err](auto) { return Err; }));
  auto It = Map.find(Decoded);
  if (It == Map.end()) {
    return Unexpect(Err);
  }
  return &It->second;
}

} // anonymous namespace

void Converter::SymbolTable::clear() {
  for (auto &Map : Names) {
    Map.clear();
  }
  Next.fill(0);
  Labels.clear();
  LabelStack.clear();
  FieldNames.clear();
}

void Converter::SymbolTable::clearLocals() {
  Names[static_cast<size_t>(IndexSpace::Local)].clear();
  next(IndexSpace::Local) = 0;
  Labels.clear();
  LabelStack.clear();
}

Expect<void> Converter::SymbolTable::define(IndexSpace Space, std::string Name,
                                            ErrCode::Value DupErr) {
  uint32_t &Idx = next(Space);
  if (!Name.empty() &&
      !Names[static_cast<size_t>(Space)].emplace(std::move(Name), Idx).second) {
    return Unexpect(DupErr);
  }
  ++Idx;
  return {};
}

Expect<void> Converter::SymbolTable::pushLabel(std::string_view Label) {
  std::string Name;
  if (!Label.empty()) {
    // An empty id, such as $ or $"", is an error, and not a block with no
    // label.
    EXPECTED_TRY(Name, decodeIdentifier(Label));
  }
  if (!Name.empty()) {
    Labels[Name].push_back(static_cast<uint32_t>(LabelStack.size()));
  }
  LabelStack.push_back(std::move(Name));
  return {};
}

void Converter::SymbolTable::popLabel() {
  auto L = std::move(LabelStack.back());
  LabelStack.pop_back();
  if (!L.empty()) {
    auto It = Labels.find(L);
    It->second.pop_back();
    if (It->second.empty()) {
      Labels.erase(It);
    }
  }
}

bool Converter::isIndexOrId(Node N) {
  const auto Type = nodeType(N);
  return Type == NodeType::U || Type == NodeType::Id;
}

Expect<void> Converter::expectIndexOrId(Node N) {
  if (likely(isIndexOrId(N))) {
    return {};
  }
  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

Expect<uint32_t>
Converter::SymbolTable::resolveIdx(const NameMap &Map,
                                   std::string_view Ref) const {
  if (!Ref.empty() && Ref[0] == '$') {
    EXPECTED_TRY(auto Idx, lookupId(Map, Ref, ErrCode::Value::WatUnknownId));
    return *Idx;
  }
  // This is a numeric index in decimal or in 0x hex. An index that does not
  // fit in a u32 is malformed. An index that is out of range of its space is
  // not a syntax error. The validator reports it, as it does for a binary
  // module.
  return parseUint32(Ref);
}

Expect<uint32_t> Converter::SymbolTable::resolve(IndexSpace Space,
                                                 std::string_view Ref) const {
  return resolveIdx(Names[static_cast<size_t>(Space)], Ref);
}

Expect<uint32_t>
Converter::SymbolTable::resolveLabel(std::string_view Ref) const {
  if (!Ref.empty() && Ref[0] == '$') {
    EXPECTED_TRY(auto Depths,
                 lookupId(Labels, Ref, ErrCode::Value::WatUnknownLabel));
    return static_cast<uint32_t>(LabelStack.size() - 1 - Depths->back());
  }
  // This is a numeric label. A label index that does not fit in 32 bits is
  // out of range, as for every other index.
  return parseUint32(Ref).map_error([](auto E) {
    return E == ErrCode::Value::WatI32ConstOutOfRange
               ? E
               : ErrCode::Value::WatUnknownLabel;
  });
}

Expect<uint32_t>
Converter::SymbolTable::resolveField(uint32_t TypeIdx,
                                     std::string_view Ref) const {
  // A numeric field index that is out of range is an error, and it does not
  // wrap. A type without field names gives an empty map.
  static const NameMap NoNames;
  auto It = FieldNames.find(TypeIdx);
  return resolveIdx(It == FieldNames.end() ? NoNames : It->second, Ref);
}

std::optional<Converter::ExternKind>
Converter::externKind(std::string_view KW) {
  using IndexSpace = SymbolTable::IndexSpace;
  static constexpr std::pair<std::string_view, ExternKind> Kinds[] = {
      {"func"sv,
       {ExternalType::Function, IndexSpace::Func,
        ErrCode::Value::WatDuplicateFunc}},
      {"table"sv,
       {ExternalType::Table, IndexSpace::Table,
        ErrCode::Value::WatDuplicateTable}},
      {"memory"sv,
       {ExternalType::Memory, IndexSpace::Memory,
        ErrCode::Value::WatDuplicateMemory}},
      {"global"sv,
       {ExternalType::Global, IndexSpace::Global,
        ErrCode::Value::WatDuplicateGlobal}},
      {"tag"sv,
       {ExternalType::Tag, IndexSpace::Tag, ErrCode::Value::WatDuplicateId}},
  };
  return lookupKeyword(Kinds, KW);
}

std::string_view Converter::nodeText(Node N) const { return N.text(Source); }

NodeType Converter::peekType(const Cursor &C) const {
  if (!C.valid()) {
    return NodeType::Unknown;
  }
  return nodeType(C.node());
}

std::string_view Converter::peekKeyword(const Cursor &C) const {
  return peekType(C) == NodeType::Keyword ? nodeText(C.node()) : ""sv;
}

bool Converter::sexprMatch(const Cursor &C, std::string_view KW) const {
  return C.valid() && sexprMatch(C.node(), KW);
}

bool Converter::sexprMatch(Node N, std::string_view KW) const {
  if (nodeType(N) != NodeType::Sexpr) {
    return false;
  }
  Cursor FC(N);
  return peekKeyword(FC) == KW;
}

Expect<uint32_t> Converter::takeIdx(Cursor &C, SymbolTable::IndexSpace Space) {
  Node Imm = C.node();
  EXPECTED_TRY(expectIndexOrId(Imm));
  EXPECTED_TRY(auto Idx, Syms.resolve(Space, nodeText(Imm)));
  C.next();
  return Idx;
}

Expect<uint32_t> Converter::takeLabelIdx(Cursor &C) {
  Node Imm = C.node();
  EXPECTED_TRY(expectIndexOrId(Imm));
  EXPECTED_TRY(auto Idx, Syms.resolveLabel(nodeText(Imm)));
  C.next();
  return Idx;
}

Expect<void> Converter::takeOptIdx(Cursor &C, SymbolTable::IndexSpace Space,
                                   uint32_t &Idx) {
  if (isIndexOrId(C.node())) {
    EXPECTED_TRY(Idx, takeIdx(C, Space));
  }
  return {};
}

Expect<std::pair<uint32_t, uint32_t>>
Converter::takeOptIdxPair(Cursor &C, SymbolTable::IndexSpace Space) {
  if (!isIndexOrId(C.node())) {
    return std::pair<uint32_t, uint32_t>{0, 0};
  }
  EXPECTED_TRY(auto First, takeIdx(C, Space));
  EXPECTED_TRY(auto Second, takeIdx(C, Space));
  return std::pair{First, Second};
}

Expect<std::pair<std::optional<uint32_t>, uint32_t>>
Converter::takeOptIdxThenIdx(Cursor &C, SymbolTable::IndexSpace First,
                             SymbolTable::IndexSpace Second) {
  Node Imm = C.node();
  EXPECTED_TRY(expectIndexOrId(Imm));
  C.next();
  if (!isIndexOrId(C.node())) {
    // The only index is the second one.
    EXPECTED_TRY(auto SecondIdx, Syms.resolve(Second, nodeText(Imm)));
    return std::pair<std::optional<uint32_t>, uint32_t>{std::nullopt,
                                                        SecondIdx};
  }
  EXPECTED_TRY(auto FirstIdx, Syms.resolve(First, nodeText(Imm)));
  EXPECTED_TRY(auto SecondIdx, takeIdx(C, Second));
  return std::pair<std::optional<uint32_t>, uint32_t>{FirstIdx, SecondIdx};
}

std::string_view Converter::takeOptionalId(Cursor &C) const {
  if (peekType(C) != NodeType::Id) {
    return {};
  }
  auto Text = nodeText(C.node());
  C.next();
  return Text;
}

Expect<std::string> Converter::takeName(Cursor &C) {
  if (peekType(C) != NodeType::String) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  EXPECTED_TRY(auto Name, parseString(nodeText(C.node())));
  C.next();
  return Name;
}

namespace {
// Give the error code for the unclosed token Text. The token starts with "(;"
// or "(@" and ends at the end of the input. The scan of an annotation follows
// the external scanner, so that it finds an unclosed string in the same way.
// The innermost token that is not closed gives the error, as in the reference
// interpreter: a comment or a string that runs to the end of the input inside
// an annotation is an unclosed comment or an unclosed string.
ErrCode::Value classifyUnclosed(std::string_view Text) noexcept {
  if (Text.substr(0, 2) != "(@"sv) {
    return ErrCode::Value::WatUnclosedComment;
  }
  size_t I = 2;
  if (I >= Text.size() ||
      isAnnotationIdEnd(static_cast<unsigned char>(Text[I]))) {
    return ErrCode::Value::WatEmptyAnnotationId;
  }
  if (Text[I] != '"') {
    // An unquoted id ends at the first character that is not an idchar, as
    // in the external scanner.
    while (I < Text.size() && isIdChar(static_cast<unsigned char>(Text[I]))) {
      ++I;
    }
  }
  bool InString = false;
  for (; I < Text.size(); ++I) {
    const char C = Text[I];
    const char Next = I + 1 < Text.size() ? Text[I + 1] : '\0';
    if (InString) {
      if (C == '\\') {
        ++I;
      } else if (C == '"') {
        InString = false;
      }
    } else if (C == '"') {
      InString = true;
    } else if (C == '(' && Next == ';') {
      bool Closed = false;
      I = blockCommentEnd(Text, I + 2, &Closed) - 1;
      if (!Closed) {
        return ErrCode::Value::WatUnclosedComment;
      }
    } else if (C == ';' && Next == ';') {
      I = lineCommentEnd(Text, I + 2);
    }
  }
  return InString ? ErrCode::Value::WatUnclosedString
                  : ErrCode::Value::WatUnclosedAnnotation;
}
} // namespace

ErrCode::Value
Converter::classifyError(Node Root, Span<const size_t> AnnotationStarts) const {
  // Descend to the first ERROR node.
  Node Err = Root;
  while (nodeType(Err) != NodeType::Error) {
    Err = Err.firstErrorChild();
    if (Err.isNull()) {
      return ErrCode::Value::WatUnexpectedToken;
    }
  }

  // The scanner gives one unclosed token for a block comment or an
  // annotation that runs to the end of the input.
  Node Unclosed;
  Err.forEachLeaf([&Unclosed](Node Leaf, bool) {
    if (Unclosed.isNull() && nodeType(Leaf) == NodeType::Unclosed) {
      Unclosed = Leaf;
    }
  });
  if (!Unclosed.isNull()) {
    return classifyUnclosed(Unclosed.text(Source));
  }

  // Find the annotation errors. In (@id ...), the id must come directly after
  // the @. The first (@ in the error node gives the error. AnnotationStarts
  // holds no (@ from a string or a comment.
  const size_t Begin = Err.startByte();
  const size_t End = Err.endByte();
  for (auto Pos : AnnotationStarts) {
    const size_t AtPos = Pos + 1;
    if (AtPos < Begin) {
      continue;
    }
    if (AtPos >= End) {
      break;
    }
    if (AtPos + 1 >= End ||
        isAnnotationIdEnd(static_cast<unsigned char>(Source[AtPos + 1]))) {
      return ErrCode::Value::WatEmptyAnnotationId;
    }
    if (Pos >= Begin) {
      return ErrCode::Value::WatUnclosedAnnotation;
    }
    break;
  }

  return ErrCode::Value::WatUnexpectedToken;
}

namespace {
// Give a key for the function type FT. Two function types have the same key
// if and only if they are equal.
std::string funcTypeKey(const AST::FunctionType &FT) {
  std::string Key;
  const auto Append = [&Key](const std::vector<ValType> &Types) {
    const auto Count = static_cast<uint32_t>(Types.size());
    Key.append(reinterpret_cast<const char *>(&Count), sizeof(Count));
    for (const auto &Type : Types) {
      Key.push_back(static_cast<char>(Type.getCode()));
      Key.push_back(static_cast<char>(Type.getHeapTypeCode()));
      const uint32_t Idx = Type.getTypeIndex();
      Key.append(reinterpret_cast<const char *>(&Idx), sizeof(Idx));
    }
  };
  Append(FT.getParamTypes());
  Append(FT.getReturnTypes());
  return Key;
}
} // namespace

uint32_t Converter::findOrCreateFuncType(AST::FunctionType FuncTy,
                                         AST::Module &Mod) {
  auto &Types = Mod.getTypeSection().getContent();
  // Add the types that came after the last call to the map. A type in a rec
  // group of two types or more has a distinct identity, even if the structure
  // is equal, so only a standalone type or a rec group of one type can be
  // used again. An inline function-type abbreviation also gives a final type
  // with no supertype. A definition that is not final, or that has a
  // supertype, has a different identity.
  for (; FuncTypesSeen < Types.size(); ++FuncTypesSeen) {
    const auto &Type = Types[FuncTypesSeen];
    if (Type.getRecursiveInfo().RecTypeSize > 1) {
      continue;
    }
    if (Type.isFinal() && Type.getSuperTypeIndices().empty() &&
        Type.getCompositeType().isFunc()) {
      FuncTypeIndices.emplace(
          funcTypeKey(Type.getCompositeType().getFuncType()),
          static_cast<uint32_t>(FuncTypesSeen));
    }
  }
  const auto NewIdx = static_cast<uint32_t>(Types.size());
  const auto [It, Inserted] =
      FuncTypeIndices.emplace(funcTypeKey(FuncTy), NewIdx);
  if (!Inserted) {
    return It->second;
  }
  Types.emplace_back(std::move(FuncTy));
  Types.back().setTypeIndex(NewIdx);
  FuncTypesSeen = Types.size();
  return NewIdx;
}

bool Converter::hasInlineSexpr(Node N, std::string_view KW) const {
  for (Cursor HC(N); HC.valid(); HC.next()) {
    if (sexprMatch(HC, KW)) {
      return true;
    }
  }
  return false;
}

// A func, table, memory, global, or tag with an inline import is an import,
// not a definition. An example is (func (import "m" "f") ...). The import
// comes after the optional id and the inline exports, as in
// convertInlineImportExport, so the test does not read the rest of the field.
bool Converter::hasInlineImport(Node N) const {
  Cursor C(N);
  C.next(); // Skip the keyword.
  takeOptionalId(C);
  while (sexprMatch(C, "export"sv)) {
    C.next();
  }
  return sexprMatch(C, "import"sv);
}

Expect<void> Converter::checkMutGlobalImport(const AST::GlobalType &GT) const {
  if (GT.getValMut() == ValMut::Var) {
    return needProposal(Proposal::ImportExportMutGlobals,
                        ErrCode::Value::InvalidMut);
  }
  return {};
}

Expect<void> Converter::needProposal(Proposal P, ErrCode::Value Code) const {
  if (likely(Conf.hasProposal(P))) {
    return {};
  }
  if (!QuietProposal) {
    spdlog::error(ErrInfo::InfoProposal(P));
  }
  return Unexpect(Code);
}

Expect<void> Converter::checkInstrProposal(OpCode Code) const {
  if (auto Res = Conf.isInstrNeedProposal(Code); unlikely(!!Res)) {
    return needProposal(*Res, ErrCode::Value::IllegalOpCode);
  }
  return {};
}

Expect<void> Converter::checkMemIdxProposal() const {
  // The binary format holds a zero byte for the memory before the Multiple
  // Memories proposal.
  return needProposal(Proposal::MultiMemories,
                      ErrCode::Value::ExpectedZeroByte);
}

ErrCode::Value Converter::refTypeFailCode() const {
  return Conf.hasProposal(Proposal::ReferenceTypes)
             ? ErrCode::Value::MalformedRefType
             : ErrCode::Value::MalformedElemType;
}

// A reserved leaf token shows two adjacent tokens with no whitespace, or a
// malformed numeric literal. HasError tells if the tree has errors. If it has,
// skip a reserved token that starts with '@'. Such a token comes from an
// annotation that did not parse, and classifyError reports it.
Expect<void>
Converter::checkReservedLeaf(bool HasError, Node Leaf,
                             Span<const size_t> AnnotationStarts) const {
  if (nodeType(Leaf) != NodeType::Reserved) {
    return {};
  }
  auto Text = Leaf.text(Source);
  if (!Text.empty() && Text.front() == '@') {
    if (HasError) {
      return {};
    }
    // The token starts with '@', and the parent holds a (@ that is not in a
    // string or a comment. The annotation scanner can fail here on an
    // unclosed string.
    const Node Parent = Leaf.parent();
    const size_t Begin = Parent.startByte();
    const size_t End = Parent.endByte();
    for (auto Pos : AnnotationStarts) {
      if (Pos < Begin) {
        continue;
      }
      if (Pos < End && classifyUnclosed(Source.substr(Pos, End - Pos)) ==
                           ErrCode::Value::WatUnclosedString) {
        return Unexpect(ErrCode::Value::WatUnclosedString);
      }
      break;
    }
    return Unexpect(ErrCode::Value::WatUnknownOperator);
  }
  // This is a bare '"', an unclosed string literal, or a quoted string that
  // the scanner did not accept. A string with an escape that is not valid,
  // such as "\q" or "\u{_1}", is one reserved token. Decode it to give the
  // error of the string.
  if (!Text.empty() && Text.front() == '"') {
    size_t Close = 1;
    while (Close < Text.size() && Text[Close] != '"') {
      Close += Text[Close] == '\\' ? 2 : 1;
    }
    if (Close >= Text.size()) {
      return Unexpect(ErrCode::Value::WatUnclosedString);
    }
    if (Close + 1 == Text.size()) {
      EXPECTED_TRY(parseString(Text));
    }
  }
  return Unexpect(ErrCode::Value::WatUnknownOperator);
}

// Find the empty annotation ids, such as (@ at the end of the source or (@(,
// that do not make an annotation node. Examine them only if the tree has no
// module content. Inside an annotation these patterns are data, not syntax,
// so a (@ in one of the AnnotationRanges does not count. Both lists are in
// source order.
Expect<void> Converter::checkBareAnnotations(
    Node Root, Span<const size_t> AnnotationStarts,
    Span<const std::pair<size_t, size_t>> AnnotationRanges) const {
  for (Cursor RC(Root); RC.valid(); RC.next()) {
    if (nodeType(RC.node()) != NodeType::Error) {
      return {};
    }
  }
  auto Range = AnnotationRanges.begin();
  for (auto Pos : AnnotationStarts) {
    while (Range != AnnotationRanges.end() && Range->second <= Pos) {
      ++Range;
    }
    if (Range != AnnotationRanges.end() && Range->first <= Pos) {
      continue;
    }
    size_t J = Pos + 2; // This position comes after the (@ prefix.
    if (J >= Source.size()) {
      // The source ends directly after the (@ prefix.
      return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
    }
    if (isAnnotationIdEnd(static_cast<unsigned char>(Source[J]))) {
      return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
    }
  }
  return {};
}

// Complete the module after pass 2. The binary loader also sets the type
// indices and the tag types when it loads a module.
void Converter::finalizeModule(AST::Module &Mod) {
  // Set TypeIndex on each type.
  auto &TypeVec = Mod.getTypeSection().getContent();
  for (uint32_t I = 0; I < TypeVec.size(); ++I) {
    TypeVec[I].setTypeIndex(I);
  }

  AST::linkTagTypes(Mod);

  // The data count section exists only with the segment ids, because the
  // binary loader rejects it before the Bulk Memory and Reference Types
  // proposals. A serialized module then loads with the same configuration.
  auto &DataSec = Mod.getDataSection().getContent();
  if (!DataSec.empty() && hasSegmentIds()) {
    Mod.getDataCountSection().setContent(static_cast<uint32_t>(DataSec.size()));
  }
}

// module ::= ( module id? modulefield* )
// This is the main entry. It converts the parse tree into an AST::Module with
// two passes: the first pass collects the indices, and the second pass builds
// the module.
Expect<AST::Module> Converter::convert(const Tree &ParseTree,
                                       std::string_view Src) {
  Source = Src;
  Syms.clear();
  Order = {};
  ImportCounts = {};
  FuncTypeIndices.clear();
  FuncTypesSeen = 0;
  ImplicitTypeUses.clear();

  std::vector<size_t> AnnotationStarts;
  EXPECTED_TRY(scanSourceCharacters(Source, &AnnotationStarts));

  Node Root = ParseTree.rootNode();
  if (Root.isNull()) {
    return Unexpect(ErrCode::Value::WatUnexpectedEnd);
  }

  const bool HasError = Root.hasError();
  {
    // Walk the leaves once. Keep the first error of each check, and report
    // them in the order of the checks.
    Expect<void> ReservedRes, AnnotationRes, AdjacentRes;
    std::vector<std::pair<size_t, size_t>> AnnotationRanges;
    Node Prev;
    Root.forEachLeaf([&](Node Leaf, bool IsAnnotation) {
      if (IsAnnotation) {
        AnnotationRanges.emplace_back(Leaf.startByte(), Leaf.endByte());
        if (AnnotationRes) {
          AnnotationRes = checkAnnotation(Leaf.text(Source));
        }
        return;
      }
      if (ReservedRes) {
        ReservedRes = checkReservedLeaf(HasError, Leaf, AnnotationStarts);
      }
      if (AdjacentRes && !HasError && !Prev.isNull() &&
          Prev.endByte() == Leaf.startByte()) {
        AdjacentRes = checkAdjacentTokens(Prev.text(Source), Leaf.text(Source));
      }
      Prev = Leaf;
    });
    EXPECTED_TRY(ReservedRes);
    EXPECTED_TRY(AnnotationRes);
    EXPECTED_TRY(
        checkBareAnnotations(Root, AnnotationStarts, AnnotationRanges));
    EXPECTED_TRY(AdjacentRes);
  }

  // The root holds one module or a bare sequence of module fields.
  EXPECTED_TRY(auto Fields, moduleFields(Root));

  // Pass 1: collect identifiers and assign indices.
  EXPECTED_TRY(collectIndices(Fields));

  // Pass 2: build AST::Module.
  AST::Module Mod;
  Mod.getMagic() = {0x00, 0x61, 0x73, 0x6D};
  Mod.getVersion() = {0x01, 0x00, 0x00, 0x00};
  CurMod = &Mod;
  auto Built = buildModule(Root, Fields, Mod);
  CurMod = nullptr;
  EXPECTED_TRY(Built);
  finalizeModule(Mod);

  // If the conversion is successful but a parse error occurred, return the
  // parse error.
  if (HasError) {
    return Unexpect(classifyError(Root, AnnotationStarts));
  }

  return Mod;
}

// module ::= ( module id? modulefield* ) | modulefield*
// The root holds exactly one (module ...) sexpr, or a bare sequence of
// module fields. A second module, a (module binary ...) or (module quote ...)
// form, and a stray token are malformed. Those forms belong to the WAST
// script, not to the module parser. An ERROR node stays for classifyError,
// which reports it with the specific code at the end.
Expect<std::vector<Node>> Converter::moduleFields(Node Root) const {
  std::vector<Node> Fields;
  Cursor C(Root);
  while (C.valid() && nodeType(C.node()) == NodeType::Error) {
    C.next();
  }
  // Append the field sexprs at FC to Fields, up to the end of FC. Skip an
  // ERROR node, which classifyError reports later. If RejectModule is true, a
  // (module ...) sexpr is malformed.
  const auto CollectFields = [&](Cursor &FC,
                                 bool RejectModule) -> Expect<void> {
    for (; FC.valid(); FC.next()) {
      auto Type = nodeType(FC.node());
      if (Type == NodeType::Sexpr) {
        if (RejectModule && sexprMatch(FC, "module"sv)) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        Fields.push_back(FC.node());
      } else if (Type != NodeType::Error) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
    }
    return {};
  };
  if (sexprMatch(C, "module"sv)) {
    Node Module = C.node();
    for (C.next(); C.valid(); C.next()) {
      if (nodeType(C.node()) != NodeType::Error) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
    }
    Cursor MC(Module);
    MC.next(); // Skip the "module" keyword.
    // The module id names nothing, but it must be a valid identifier.
    if (const auto Id = takeOptionalId(MC); !Id.empty()) {
      EXPECTED_TRY(decodeIdentifier(Id));
    }
    EXPECTED_TRY(CollectFields(MC, false));
    return Fields;
  }
  EXPECTED_TRY(CollectFields(C, true));
  return Fields;
}

// Pass 1 walks the module fields and assigns an index to each named
// definition.
Expect<void> Converter::collectIndices(Span<const Node> Fields) {
  for (const auto &Field : Fields) {
    EXPECTED_TRY(collectModuleField(Field));
  }
  return {};
}

// modulefield ::= type | import | func | table | memory | global | export
//               | start | elem | data | tag
// Register the identifiers of one field and assign its index-space positions.
Expect<void> Converter::collectModuleField(Node Field) {
  // Field must be a sexpr, and a keyword must come first.
  Cursor FC(Field);
  auto KWNode = FC.node();
  if (nodeType(KWNode) != NodeType::Keyword) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  auto KW = nodeText(KWNode);
  FC.next(); // Move FC to the position after the first keyword.

  // Extract the optional id that comes directly after the keyword. The caller
  // must first move the cursor past the keyword. On success, this lambda
  // consumes the Id node.
  auto GetId = [&](Cursor &C) -> Expect<std::string> {
    if (const auto Id = takeOptionalId(C); !Id.empty()) {
      return decodeIdentifier(Id);
    }
    return std::string();
  };

  using IndexSpace = SymbolTable::IndexSpace;
  // Take the optional id at C, and give it the next index of Space.
  auto Define = [&](Cursor &C, IndexSpace Space,
                    ErrCode::Value Err) -> Expect<void> {
    EXPECTED_TRY(auto Id, GetId(C));
    return Syms.define(Space, std::move(Id), Err);
  };

  if (KW == "type"sv) {
    EXPECTED_TRY(Define(FC, IndexSpace::Type, ErrCode::Value::WatDuplicateId));
  } else if (KW == "rec"sv) {
    // This is a recursive type. Each child (type ...) sexpr gets an index.
    Cursor RC(Field);
    for (; RC.valid(); RC.next()) {
      Node Child = RC.node();
      if (nodeType(Child) == NodeType::Sexpr) {
        Cursor ChildC(Child);
        Node Imm = ChildC.node();
        if (nodeType(Imm) != NodeType::Keyword) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        if (nodeText(Imm) == "type"sv) {
          ChildC.next();
          EXPECTED_TRY(
              Define(ChildC, IndexSpace::Type, ErrCode::Value::WatDuplicateId));
        }
      }
    }
  } else if (KW == "import"sv) {
    // An import consumes an index in its own space. Find the inner descriptor
    // sexpr, which is func, table, memory, global, or tag.
    Cursor IC(Field);
    while (IC.valid()) {
      Node Child = IC.node();
      IC.next();
      if (nodeType(Child) != NodeType::Sexpr) {
        continue;
      }
      Cursor ChildC(Child);
      if (peekType(ChildC) != NodeType::Keyword) {
        continue;
      }
      auto Kind = externKind(nodeText(ChildC.node()));
      ChildC.next(); // Move past the descriptor keyword.
      EXPECTED_TRY(auto Id, GetId(ChildC));
      if (!Kind) {
        return Unexpect(ErrCode::Value::WatUnknownOperator);
      }
      EXPECTED_TRY(Syms.define(Kind->Space, std::move(Id), Kind->DupErr));
    }
  } else if (auto Kind = externKind(KW)) {
    EXPECTED_TRY(Define(FC, Kind->Space, Kind->DupErr));
    // The inline (elem ...) abbreviation of a table makes an element segment,
    // and the segment takes an index. The segments after it must count it.
    // The inline (data ...) abbreviation of a memory makes a data segment in
    // the same way.
    if (Kind->Type == ExternalType::Table && hasInlineSexpr(Field, "elem"sv)) {
      Syms.next(IndexSpace::Elem)++;
    } else if (Kind->Type == ExternalType::Memory &&
               hasInlineSexpr(Field, "data"sv)) {
      Syms.next(IndexSpace::Data)++;
    }
  } else if (KW == "elem"sv || KW == "data"sv) {
    // The id after elem or data is the id of the segment. Before the Bulk
    // Memory and Reference Types proposals, a segment has no id, and the id
    // is the table or the memory. Pass 2 resolves that target.
    const auto Space = KW == "elem"sv ? IndexSpace::Elem : IndexSpace::Data;
    std::string Id;
    if (hasSegmentIds()) {
      EXPECTED_TRY(Id, GetId(FC));
    }
    EXPECTED_TRY(Syms.define(Space, std::move(Id),
                             KW == "elem"sv
                                 ? ErrCode::Value::WatDuplicateElem
                                 : ErrCode::Value::WatDuplicateData));
  }
  // The other fields, such as export and start, do not define a name. Skip
  // them.
  return {};
}

// Pass 2 converts each module field into AST sections. It converts the types
// first. Then it converts all the other fields in the order of the source.
Expect<void> Converter::buildModule(Node Root, Span<const Node> Fields,
                                    AST::Module &Mod) {
  // Convert the type definitions first. An explicit type must get its correct
  // index before the converter makes the implicit function types.
  for (const auto &Field : Fields) {
    Cursor FC(Field);
    auto KW = peekKeyword(FC);
    if (KW == "type"sv || KW == "rec"sv) {
      EXPECTED_TRY(buildModuleField(Field, Mod));
    }
  }

  // Then make the implicit function types of every inline typeuse, in the
  // order of the source. A numeric type index can point forward to such a
  // type, so the types must exist before the fields resolve them.
  QuietProposal = true;
  collectImplicitTypes(Root, Mod);
  QuietProposal = false;

  // Then convert all the other fields in the order of the source. The types
  // are already complete.
  for (const auto &Field : Fields) {
    Cursor FC(Field);
    auto KW = peekKeyword(FC);
    if (KW == "type"sv || KW == "rec"sv) {
      continue;
    }
    EXPECTED_TRY(buildModuleField(Field, Mod));
  }

  return {};
}

// The spec appends the implicit type of an inline typeuse to the type
// section, in the order of appearance. This pass walks the whole tree in
// that order and makes each implicit type before pass 2 converts the fields.
// Pass 2 then finds the same type at the same index, because
// findOrCreateFuncType uses the existing type again.
//
// A typeuse comes after these keywords:
// - func and tag, as a module field or as an import description.
// - call_indirect and return_call_indirect.
// - block, loop, if, and try_table, as the block type.
// The flat form and the folded form both put the (type ...), (param ...), and
// (result ...) sexprs after the keyword. As a result, one scan of the
// children of each sexpr covers both forms.
//
// A typeuse with a (type ...) clause and a block type with no (param ...) or
// (result ...) clause make no implicit type, so the walk skips them. The walk
// keeps the type index of each inline typeuse in ImplicitTypeUses, so that
// pass 2 does not resolve it again.
//
// The walk uses an explicit stack, so the depth of the tree does not bound
// it. An error in a typeuse is not an error here. Pass 2 reports it with the
// correct code.
void Converter::collectImplicitTypes(Node Root, AST::Module &Mod) {
  struct Frame {
    Cursor C;
    std::string_view Own;    // The first keyword of this sexpr.
    std::string_view Parent; // The first keyword of the parent sexpr.
    uint32_t Index;          // The index of the next child.
  };
  std::vector<Frame> Stack;
  Stack.push_back({Cursor(Root), ""sv, ""sv, 0});
  while (!Stack.empty()) {
    auto &Top = Stack.back();
    if (!Top.C.valid()) {
      Stack.pop_back();
      continue;
    }
    Node Child = Top.C.node();
    const auto Type = nodeType(Child);
    const auto Index = Top.Index++;
    const auto Parent = Top.Parent;
    if (Type == NodeType::Sexpr) {
      Cursor FC(Child);
      auto KW = peekKeyword(FC);
      auto Own = Top.Own;
      Top.C.next();
      Stack.push_back({std::move(FC), KW, Own, 0});
      continue;
    }
    // Advance first, so that the typeuse scan starts after the keyword. The
    // scan works on a copy.
    Top.C.next();
    if (Type != NodeType::Keyword) {
      continue;
    }
    auto KW = nodeText(Child);
    if (KW == "func"sv || KW == "tag"sv) {
      // A definition or an import description holds the keyword first. The
      // (func ...) inside a (type ...) or a (sub ...) is a comptype instead.
      if (Index != 0 ||
          (Parent != "module"sv && Parent != "import"sv && !Parent.empty())) {
        continue;
      }
      auto TC = Top.C.copy();
      if (peekType(TC) == NodeType::Id) {
        TC.next();
      }
      while (sexprMatch(TC, "export"sv) || sexprMatch(TC, "import"sv)) {
        TC.next();
      }
      collectTypeUse(TC, Mod, ParamIds::Distinct);
    } else if (KW == "call_indirect"sv || KW == "return_call_indirect"sv) {
      auto TC = Top.C.copy();
      if (isIndexOrId(TC.node())) {
        TC.next();
      }
      collectTypeUse(TC, Mod, ParamIds::None);
    } else if (KW == "block"sv || KW == "loop"sv || KW == "if"sv ||
               KW == "try_table"sv) {
      auto TC = Top.C.copy();
      if (peekType(TC) == NodeType::Id) {
        TC.next();
      }
      if (!atInlineClause(TC)) {
        continue;
      }
      const auto Key = TC.node().startByte();
      if (auto BType = parseBlockType(TC);
          BType && !BType->isEmpty() && !BType->isValType()) {
        ImplicitTypeUses.emplace(Key, BType->getTypeIndex());
      }
    }
  }
}

// Resolve the typeuse at C for collectImplicitTypes. A typeuse with no
// clause makes the type [] -> [], and pass 2 resolves it again at little
// cost. Keep the type index of an inline typeuse.
void Converter::collectTypeUse(Cursor &C, AST::Module &Mod, ParamIds Ids) {
  if (sexprMatch(C, "type"sv)) {
    return;
  }
  if (!atInlineClause(C)) {
    static_cast<void>(resolveTypeUse(C, Mod, Ids));
    return;
  }
  const auto Key = C.node().startByte();
  if (auto Idx = resolveTypeUse(C, Mod, Ids)) {
    ImplicitTypeUses.emplace(Key, *Idx);
  }
}

bool Converter::atInlineClause(const Cursor &C) const {
  return sexprMatch(C, "param"sv) || sexprMatch(C, "result"sv);
}

std::optional<uint32_t> Converter::takeImplicitTypeUse(Cursor &C) {
  if (!atInlineClause(C)) {
    return std::nullopt;
  }
  auto It = ImplicitTypeUses.find(C.node().startByte());
  if (It == ImplicitTypeUses.end()) {
    return std::nullopt;
  }
  // collectImplicitTypes parsed these clauses with no error, so they are only
  // (param ...) and (result ...) clauses.
  while (atInlineClause(C)) {
    C.next();
  }
  return It->second;
}

// Send one module field to the correct converter. The keyword selects the
// converter.
Expect<void> Converter::buildModuleField(Node Field, AST::Module &Mod) {
  Cursor FC(Field);
  if (peekType(FC) != NodeType::Keyword) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  auto KW = nodeText(FC.node());
  FC.next();

  // Examine the order of the imports. Every import must come before every
  // definition.
  auto CheckImportOrder = [this]() -> Expect<void> {
    if (Order.HasFuncDef) {
      return Unexpect(ErrCode::Value::WatImportAfterFunc);
    }
    if (Order.HasTableDef) {
      return Unexpect(ErrCode::Value::WatImportAfterTable);
    }
    if (Order.HasMemoryDef) {
      return Unexpect(ErrCode::Value::WatImportAfterMemory);
    }
    if (Order.HasGlobalDef) {
      return Unexpect(ErrCode::Value::WatImportAfterGlobal);
    }
    if (Order.HasTagDef) {
      return Unexpect(ErrCode::Value::WatImportAfterTag);
    }
    return {};
  };
  // A field with an inline import is an import. Any other field is a
  // definition, and it sets HasDef.
  auto CheckImportOrDef = [&](bool &HasDef) -> Expect<void> {
    if (hasInlineImport(Field)) {
      return CheckImportOrder();
    }
    HasDef = true;
    return {};
  };

  if (KW == "type"sv) {
    return convertTypedef(Field, Mod);
  }
  if (KW == "rec"sv) {
    return convertRecType(Field, Mod);
  }
  if (KW == "import"sv) {
    EXPECTED_TRY(CheckImportOrder());
    return convertImport(Field, Mod);
  }
  if (KW == "export"sv) {
    return convertExport(Field, Mod);
  }
  if (KW == "func"sv) {
    EXPECTED_TRY(CheckImportOrDef(Order.HasFuncDef));
    return convertFunc(Field, Mod);
  }
  if (KW == "table"sv) {
    EXPECTED_TRY(CheckImportOrDef(Order.HasTableDef));
    return convertTable(Field, Mod);
  }
  if (KW == "memory"sv) {
    EXPECTED_TRY(CheckImportOrDef(Order.HasMemoryDef));
    return convertMemory(Field, Mod);
  }
  if (KW == "global"sv) {
    EXPECTED_TRY(CheckImportOrDef(Order.HasGlobalDef));
    return convertGlobal(Field, Mod);
  }
  if (KW == "start"sv) {
    if (Order.HasStart) {
      return Unexpect(ErrCode::Value::WatMultipleStart);
    }
    Order.HasStart = true;
    return convertStart(Field, Mod);
  }
  if (KW == "elem"sv) {
    return convertElem(Field, Mod);
  }
  if (KW == "data"sv) {
    return convertData(Field, Mod);
  }
  if (KW == "tag"sv) {
    EXPECTED_TRY(CheckImportOrDef(Order.HasTagDef));
    return convertTag(Field, Mod);
  }
  // Any other keyword is a module field that the converter does not know.
  // Reject it, and do not drop it without a message. For example,
  // `(module (bogus))` must not parse as valid. An annotation is a tree-sitter
  // extra and never gets to this point. The code before this point rejects a
  // first token that is not a keyword.
  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

} // namespace WAT
} // namespace WasmEdge
