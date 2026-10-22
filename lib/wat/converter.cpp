// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "converter.h"
#include "common/errcode.h"
#include "common/errinfo.h"
#include "common/spdlog.h"
#include "wat/wat_util.h"

#include <cctype>
#include <optional>
#include <utility>
#include <vector>

using namespace std::string_view_literals;

namespace WasmEdge {
namespace WAT {

namespace {

// Classify a non-ASCII byte sequence at Src[Pos] and move Pos past it.
// For valid UTF-8, the result is WatIllegalCharacter. Such a sequence is legal
// Unicode, but WAT does not permit non-ASCII outside a string. For bytes that
// are not valid, the result is MalformedUTF8.
ErrCode::Value classifyNonASCII(std::string_view Src, size_t &Pos) {
  if (const size_t Len = utf8SequenceLength(Src, Pos); Len != 0) {
    Pos += Len;
    return ErrCode::Value::WatIllegalCharacter;
  }
  ++Pos;
  return ErrCode::Value::MalformedUTF8;
}

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

Expect<uint32_t> Converter::SymbolTable::resolveIdx(const NameMap &Map,
                                                    std::string_view Ref,
                                                    ErrCode::Value Err) const {
  if (Ref.empty()) {
    return Unexpect(Err);
  }
  if (Ref[0] == '$') {
    EXPECTED_TRY(auto Idx, lookupId(Map, Ref, ErrCode::Value::WatUnknownId));
    return *Idx;
  }
  // This is a numeric index in decimal or in 0x hex.
  return parseUint32(Ref).map_error([Err](auto) { return Err; });
}

Expect<uint32_t> Converter::SymbolTable::resolve(IndexSpace Space,
                                                 std::string_view Ref) const {
  // The error code of a malformed numeric index of each space. A numeric index
  // that is out of range is not a syntax error. The validator reports it, as
  // it does for a binary module. An unknown name is a syntax error.
  static constexpr std::array<ErrCode::Value, SpaceCount> InvalidIdx = {
      ErrCode::Value::InvalidFuncTypeIdx, ErrCode::Value::InvalidFuncIdx,
      ErrCode::Value::InvalidTableIdx,    ErrCode::Value::InvalidMemoryIdx,
      ErrCode::Value::InvalidGlobalIdx,   ErrCode::Value::InvalidTagIdx,
      ErrCode::Value::InvalidElemIdx,     ErrCode::Value::InvalidDataIdx,
      ErrCode::Value::InvalidLocalIdx,
  };
  const auto I = static_cast<size_t>(Space);
  return resolveIdx(Names[I], Ref, InvalidIdx[I]);
}

Expect<uint32_t>
Converter::SymbolTable::resolveLabel(std::string_view Ref) const {
  if (!Ref.empty() && Ref[0] == '$') {
    EXPECTED_TRY(auto Depths,
                 lookupId(Labels, Ref, ErrCode::Value::WatUnknownLabel));
    return static_cast<uint32_t>(LabelStack.size() - 1 - Depths->back());
  }
  // This is a numeric label.
  return parseUint32(Ref).map_error(
      [](auto) { return ErrCode::Value::WatUnknownLabel; });
}

Expect<uint32_t>
Converter::SymbolTable::resolveField(uint32_t TypeIdx,
                                     std::string_view Ref) const {
  // A numeric field index that is out of range is an error, and it does not
  // wrap. A type without field names gives an empty map.
  static const NameMap NoNames;
  auto It = FieldNames.find(TypeIdx);
  return resolveIdx(It == FieldNames.end() ? NoNames : It->second, Ref,
                    ErrCode::Value::WatUnknownId);
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

Expect<std::pair<uint32_t, uint32_t>>
Converter::takeOptIdxThenIdx(Cursor &C, SymbolTable::IndexSpace First,
                             SymbolTable::IndexSpace Second) {
  Node Imm = C.node();
  EXPECTED_TRY(expectIndexOrId(Imm));
  C.next();
  if (!isIndexOrId(C.node())) {
    // The only index is the second one.
    EXPECTED_TRY(auto SecondIdx, Syms.resolve(Second, nodeText(Imm)));
    return std::pair<uint32_t, uint32_t>{0, SecondIdx};
  }
  EXPECTED_TRY(auto FirstIdx, Syms.resolve(First, nodeText(Imm)));
  EXPECTED_TRY(auto SecondIdx, takeIdx(C, Second));
  return std::pair{FirstIdx, SecondIdx};
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

ErrCode::Value Converter::classifyError(Node Root) const {
  // Descend to the first ERROR node.
  Node Err = Root;
  while (nodeType(Err) != NodeType::Error) {
    Err = Err.firstErrorChild();
    if (Err.isNull()) {
      return ErrCode::Value::WatUnexpectedToken;
    }
  }

  auto Text = Err.text(Source);

  // Find the annotation errors. In (@id ...), the id must come directly after
  // the @.
  auto AtPos = Text.find('@');
  if (AtPos != std::string_view::npos) {
    auto After = Text.substr(AtPos + 1);
    if (After.empty() ||
        isAnnotationIdEnd(static_cast<unsigned char>(After.front()))) {
      return ErrCode::Value::WatEmptyAnnotationId;
    }
    if (AtPos > 0 && Text[AtPos - 1] == '(') {
      return ErrCode::Value::WatUnclosedAnnotation;
    }
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
    if (auto RI = Type.getRecursiveInfo();
        RI.has_value() && RI->RecTypeSize > 1) {
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

Expect<void> Converter::checkMemIdxProposal(uint32_t MemIdx) const {
  // The binary format holds a zero byte for the memory before the Multiple
  // Memories proposal.
  if (MemIdx != 0) {
    return needProposal(Proposal::MultiMemories,
                        ErrCode::Value::ExpectedZeroByte);
  }
  return {};
}

ErrCode::Value Converter::refTypeFailCode() const {
  return Conf.hasProposal(Proposal::ReferenceTypes)
             ? ErrCode::Value::MalformedRefType
             : ErrCode::Value::MalformedElemType;
}

// Scan early for illegal characters and for malformed UTF-8. Outside a string
// and a comment, WAT does not permit a control character, DEL, or non-ASCII. A
// comment and a string can hold any UTF-8, but the converter always rejects
// malformed UTF-8.
// The scan also collects the positions of every "(@" that is outside a string
// and outside a comment. The test for a bare empty annotation id uses them. A
// raw search of the source also finds a "(@" inside a comment or a string,
// where it is data.
Expect<void>
Converter::scanSourceCharacters(std::vector<size_t> &AnnotationStarts) const {
  auto *Data = reinterpret_cast<const unsigned char *>(Source.data());
  size_t Size = Source.size();
  bool InString = false;
  for (size_t I = 0; I < Size; ++I) {
    auto C = Data[I];
    if (InString) {
      if (C >= 0x80) {
        // In a string, valid non-ASCII UTF-8 is permitted, but malformed
        // UTF-8 is not permitted.
        auto Err = classifyNonASCII(Source, I);
        if (Err == ErrCode::Value::MalformedUTF8) {
          return Unexpect(Err);
        }
        // classifyNonASCII moved I past the sequence. Adjust I for the ++I of
        // the loop.
        --I;
        continue;
      }
      if (C == '"') {
        // If the count of the backslashes before the quote is even, then the
        // quote is real and has no escape. A count of 0 is even.
        size_t Backslashes = 0;
        while (Backslashes < I && Source[I - 1 - Backslashes] == '\\') {
          ++Backslashes;
        }
        if (Backslashes % 2 == 0) {
          InString = false;
        }
      }
      continue;
    }
    // Skip a comment. A line comment starts with ;; and ends at the end of
    // the line. A block comment has the form (; ... ;), and it can nest. Any
    // character can be in a comment, but the text must be valid UTF-8.
    const bool LineComment = C == ';' && I + 1 < Size && Data[I + 1] == ';';
    if (LineComment || (C == '(' && I + 1 < Size && Data[I + 1] == ';')) {
      const size_t End = LineComment ? lineCommentEnd(Source, I + 2)
                                     : blockCommentEnd(Source, I + 2);
      if (!isValidUTF8(Source.substr(I + 2, End - I - 2))) {
        return Unexpect(ErrCode::Value::MalformedUTF8);
      }
      // Adjust I for the ++I of the outer loop. The loop then also skips the
      // newline at the end of a line comment, which is whitespace.
      I = LineComment ? End : End - 1;
      continue;
    }
    if (C == '"') {
      InString = true;
      continue;
    }
    if (C == '(' && I + 1 < Size && Data[I + 1] == '@') {
      AnnotationStarts.push_back(I);
    }
    if (C >= 0x80) {
      return Unexpect(classifyNonASCII(Source, I));
    }
    if (isControlChar(C) && !isWatSpace(C)) {
      return Unexpect(ErrCode::Value::WatIllegalCharacter);
    }
  }
  return {};
}

// A reserved leaf token shows two adjacent tokens with no whitespace, or a
// malformed numeric literal. HasError tells if the tree has errors. If it has,
// skip a reserved token that starts with '@'. Such a token comes from an
// annotation that did not parse, and classifyError reports it.
Expect<void> Converter::checkReservedLeaf(bool HasError, Node Leaf) const {
  if (nodeType(Leaf) != NodeType::Reserved) {
    return {};
  }
  auto Text = Leaf.text(Source);
  if (!Text.empty() && Text.front() == '@') {
    if (HasError) {
      return {};
    }
    auto ParentText = Leaf.parent().text(Source);
    // The token starts with '@', and the parent holds "(@". The annotation
    // scanner can fail here on an unclosed string.
    if (auto AtPos = ParentText.find("(@"sv); AtPos != std::string_view::npos) {
      auto QuotePos = ParentText.find('"', AtPos);
      if (QuotePos != std::string_view::npos) {
        auto CloseQuote = ParentText.find('"', QuotePos + 1);
        if (CloseQuote == std::string_view::npos) {
          return Unexpect(ErrCode::Value::WatUnclosedString);
        }
      }
    }
    return Unexpect(ErrCode::Value::WatUnknownOperator);
  }
  // This is a bare '"' or an unclosed string literal.
  if (!Text.empty() && Text.front() == '"') {
    auto Close = Text.find('"', 1);
    if (Close == std::string_view::npos) {
      return Unexpect(ErrCode::Value::WatUnclosedString);
    }
  }
  return Unexpect(ErrCode::Value::WatUnknownOperator);
}

// Examine the id of the annotation Ann. Reject an empty id, such as (@), (@ x),
// or (@""), a control character, and an id that does not decode to valid
// UTF-8. The scan works on the annotation nodes of the tree, so a (@ inside a
// comment, a string, a name, or the payload of an annotation does not count.
Expect<void> Converter::checkAnnotationId(Node Ann) const {
  auto Text = Ann.text(Source);
  if (Text.size() <= 2 ||
      isAnnotationIdEnd(static_cast<unsigned char>(Text[2]))) {
    return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
  }
  if (Text[2] != '"') {
    return {};
  }
  // The scanner already found the closing quote of the id, and the string
  // ends before the closing parenthesis of the annotation.
  const size_t Start = Ann.startByte() + 2; // The position of the quote.
  size_t J = Start + 1;
  if (J < Source.size() && Source[J] == '"') {
    return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
  }
  while (J < Source.size() && Source[J] != '"') {
    if (isControlChar(static_cast<unsigned char>(Source[J]))) {
      return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
    }
    // Skip the character after a backslash, so that an escaped quote does
    // not end the id.
    J += Source[J] == '\\' ? 2 : 1;
  }
  // An id is a name, so its decoded bytes must be valid UTF-8. A \HH escape
  // can give a byte of a multi-byte sequence.
  if (!parseString(Source.substr(Start, J + 1 - Start),
                   /*ValidateUTF8=*/true)) {
    return Unexpect(ErrCode::Value::MalformedUTF8);
  }
  return {};
}

// Find the empty annotation ids, such as (@ at the end of the source or (@(,
// that do not make an annotation node. Examine them only if the tree has no
// module content. Inside an annotation these patterns are data, not syntax.
Expect<void>
Converter::checkBareAnnotations(Node Root,
                                Span<const size_t> AnnotationStarts) const {
  for (Cursor RC(Root); RC.valid(); RC.next()) {
    if (nodeType(RC.node()) != NodeType::Error) {
      return {};
    }
  }
  for (auto Pos : AnnotationStarts) {
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

// Two tokens need whitespace between them. Tree-sitter can accept two adjacent
// tokens, for example "i32.const0" as "i32.const" and "0". The WAT spec does
// not permit this. Prev and Curr are consecutive leaf tokens. The caller skips
// this test if the tree already has a deferred parse error that is more
// specific.
Expect<void> Converter::checkAdjacentTokens(Node Prev, Node Curr) const {
  if (Prev.endByte() != Curr.startByte()) {
    return {};
  }
  // These two leaf tokens are adjacent and have no gap. A '(' and a ')' can
  // touch any token.
  auto PrevText = Prev.text(Source);
  auto CurrText = Curr.text(Source);
  if (PrevText == "("sv || PrevText == ")"sv || CurrText == "("sv ||
      CurrText == ")"sv) {
    return {};
  }
  // Two adjacent tokens that are not parentheses are malformed. This test
  // also rejects two adjacent strings such as "a""b". A SIMD lane needs an
  // integer. At such a position, a pattern that looks like a float, for
  // example "1" and ".5", gives an unexpected token. It does not give an
  // unknown operator.
  const auto IsDigit = [](char Ch) {
    return std::isdigit(static_cast<unsigned char>(Ch)) != 0;
  };
  if (!PrevText.empty() && !CurrText.empty() &&
      ((IsDigit(PrevText.back()) && CurrText.front() == '.') ||
       (PrevText.back() == '.' && IsDigit(CurrText.front())))) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  return Unexpect(ErrCode::Value::WatUnknownOperator);
}

// Complete the module after pass 2. The binary loader does these steps when it
// loads a module.
void Converter::finalizeModule(AST::Module &Mod) {
  // Set TypeIndex on each type.
  auto &TypeVec = Mod.getTypeSection().getContent();
  for (uint32_t I = 0; I < TypeVec.size(); ++I) {
    TypeVec[I].setTypeIndex(I);
  }

  // Set the DefType pointers on the tag types of the tag section and of the
  // import section, as setTagFunctionType() of the binary loader does.
  auto SetTagDefType = [&TypeVec](AST::TagType &TgType) {
    if (auto TypeIdx = TgType.getTypeIdx(); TypeIdx < TypeVec.size()) {
      TgType.setDefType(&TypeVec[TypeIdx]);
    }
  };
  for (auto &TgType : Mod.getTagSection().getContent()) {
    SetTagDefType(TgType);
  }
  for (auto &Desc : Mod.getImportSection().getContent()) {
    if (Desc.getExternalType() == ExternalType::Tag) {
      SetTagDefType(Desc.getExternalTagType());
    }
  }

  // If the module has data segments, set the data count section.
  auto &DataSec = Mod.getDataSection().getContent();
  if (!DataSec.empty()) {
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

  std::vector<size_t> AnnotationStarts;
  EXPECTED_TRY(scanSourceCharacters(AnnotationStarts));

  Node Root = ParseTree.rootNode();
  if (Root.isNull()) {
    return Unexpect(ErrCode::Value::WatUnexpectedEnd);
  }

  const bool HasError = Root.hasError();
  {
    // Walk the leaves once. Keep the first error of each check, and report
    // them in the order of the checks.
    Expect<void> ReservedRes, AnnotationRes, AdjacentRes;
    Node Prev;
    Root.forEachLeaf([&](Node Leaf, bool IsAnnotation) {
      if (IsAnnotation) {
        if (AnnotationRes) {
          AnnotationRes = checkAnnotationId(Leaf);
        }
        return;
      }
      if (ReservedRes) {
        ReservedRes = checkReservedLeaf(HasError, Leaf);
      }
      if (AdjacentRes && !HasError && !Prev.isNull()) {
        AdjacentRes = checkAdjacentTokens(Prev, Leaf);
      }
      Prev = Leaf;
    });
    EXPECTED_TRY(ReservedRes);
    EXPECTED_TRY(AnnotationRes);
    EXPECTED_TRY(checkBareAnnotations(Root, AnnotationStarts));
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
    return Unexpect(classifyError(Root));
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
    EXPECTED_TRY(
        Syms.define(Space, std::move(Id), ErrCode::Value::WatDuplicateId));
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
      static_cast<void>(resolveTypeUse(TC, Mod));
    } else if (KW == "call_indirect"sv || KW == "return_call_indirect"sv) {
      auto TC = Top.C.copy();
      if (isIndexOrId(TC.node())) {
        TC.next();
      }
      static_cast<void>(resolveTypeUse(TC, Mod, /*AcceptParamId=*/false));
    } else if (KW == "block"sv || KW == "loop"sv || KW == "if"sv ||
               KW == "try_table"sv) {
      auto TC = Top.C.copy();
      if (peekType(TC) == NodeType::Id) {
        TC.next();
      }
      static_cast<void>(parseBlockType(TC));
    }
  }
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
