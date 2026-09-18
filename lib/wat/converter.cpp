// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "converter.h"
#include "common/errcode.h"
#include "wat/wat_util.h"

#include <tree_sitter/api.h>

#include <cctype>
#include <cstring>
#include <limits>
#include <vector>

using namespace std::string_view_literals;

namespace WasmEdge {
namespace WAT {

namespace {

// Classify a non-ASCII byte sequence at Data[Pos] and move Pos past it.
// For valid UTF-8, the result is WatIllegalCharacter. Such a sequence is legal
// Unicode, but WAT does not permit non-ASCII outside a string. For bytes that
// are not valid, the result is MalformedUTF8.
ErrCode::Value classifyNonASCII(const unsigned char *Data, size_t Size,
                                size_t &Pos) {
  unsigned char B = Data[Pos];
  unsigned Expect;
  uint32_t CP;
  if ((B & 0xE0) == 0xC0) {
    Expect = 1;
    CP = B & 0x1F;
  } else if ((B & 0xF0) == 0xE0) {
    Expect = 2;
    CP = B & 0x0F;
  } else if ((B & 0xF8) == 0xF0) {
    Expect = 3;
    CP = B & 0x07;
  } else {
    // This is a lone continuation byte or a lead byte that is not valid.
    ++Pos;
    return ErrCode::Value::MalformedUTF8;
  }
  if (Pos + Expect >= Size) {
    ++Pos;
    return ErrCode::Value::MalformedUTF8;
  }
  for (unsigned J = 1; J <= Expect; ++J) {
    if ((Data[Pos + J] & 0xC0) != 0x80) {
      ++Pos;
      return ErrCode::Value::MalformedUTF8;
    }
    CP = (CP << 6) | (Data[Pos + J] & 0x3F);
  }
  // Reject an overlong encoding and a code point that is out of range.
  if ((Expect == 1 && CP < 0x80) || (Expect == 2 && CP < 0x800) ||
      (Expect == 3 && CP < 0x10000) || CP > 0x10FFFF ||
      (CP >= 0xD800 && CP <= 0xDFFF)) {
    ++Pos;
    return ErrCode::Value::MalformedUTF8;
  }
  Pos += Expect + 1;
  return ErrCode::Value::WatIllegalCharacter;
}

} // anonymous namespace

void Converter::SymbolTable::clear() {
  Types.clear();
  Funcs.clear();
  Tables.clear();
  Memories.clear();
  Globals.clear();
  Tags.clear();
  Elems.clear();
  Datas.clear();
  Locals.clear();
  Labels.clear();
  LabelStack.clear();
  NextType = NextFunc = NextTable = NextMemory = 0;
  NextGlobal = NextTag = NextElem = NextData = NextLocal = 0;
}

void Converter::SymbolTable::clearLocals() {
  Locals.clear();
  Labels.clear();
  LabelStack.clear();
  NextLocal = 0;
}

void Converter::SymbolTable::pushLabel(std::string_view Label) {
  std::string Name;
  if (!Label.empty()) {
    Name = decodeIdentifier(Label).value_or(std::string());
  }
  if (!Name.empty()) {
    Labels[Name].push_back(static_cast<uint32_t>(LabelStack.size()));
  }
  LabelStack.push_back(std::move(Name));
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

Expect<void> Converter::SymbolTable::isIndexOrId(Node N) {
  const auto Type = nodeType(N);
  if (likely(Type == NodeType::U || Type == NodeType::Id)) {
    return {};
  }
  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

Expect<uint32_t> Converter::SymbolTable::resolveIdx(
    const std::unordered_map<std::string, uint32_t, Hash::Hash> &Map,
    std::string_view Ref, ErrCode::Value Err) const {
  if (Ref.empty()) {
    return Unexpect(Err);
  }
  if (Ref[0] == '$') {
    EXPECTED_TRY(auto Decoded, decodeIdentifier(Ref).map_error([](auto) {
      return ErrCode::Value::WatUnknownId;
    }));
    auto It = Map.find(Decoded);
    if (It == Map.end()) {
      return Unexpect(ErrCode::Value::WatUnknownId);
    }
    return It->second;
  }
  // This is a numeric index in decimal or in 0x hex.
  uint32_t Idx = 0;
  size_t Pos = 0;
  bool IsHex = false;
  if (Ref.size() >= 2 && Ref[0] == '0' && (Ref[1] == 'x' || Ref[1] == 'X')) {
    IsHex = true;
    Pos = 2;
  }
  for (; Pos < Ref.size(); ++Pos) {
    char C = Ref[Pos];
    if (C == '_') {
      continue;
    }
    int D = IsHex ? hexDigit(C) : (C >= '0' && C <= '9' ? C - '0' : -1);
    if (D < 0) {
      return Unexpect(Err);
    }
    uint32_t Base = IsHex ? 16 : 10;
    if (Idx > std::numeric_limits<uint32_t>::max() / Base ||
        (Idx == std::numeric_limits<uint32_t>::max() / Base &&
         static_cast<uint32_t>(D) >
             std::numeric_limits<uint32_t>::max() % Base)) {
      return Unexpect(Err);
    }
    Idx = Idx * Base + static_cast<uint32_t>(D);
  }
  return Idx;
}

Expect<uint32_t>
Converter::SymbolTable::resolveType(std::string_view Ref) const {
  EXPECTED_TRY(auto Idx,
               resolveIdx(Types, Ref, ErrCode::Value::InvalidFuncTypeIdx));
  // Examine the bounds of a numeric type index. NextType increases, because
  // resolveTypeUse makes implicit types.
  if (!Ref.empty() && Ref.front() != '$' && Idx >= NextType) {
    return Unexpect(ErrCode::Value::WatUnknownType);
  }
  return Idx;
}
Expect<uint32_t> Converter::SymbolTable::resolve(IndexSpace Space,
                                                 std::string_view Ref) const {
  switch (Space) {
  case IndexSpace::Func:
    return resolveIdx(Funcs, Ref, ErrCode::Value::InvalidFuncIdx);
  case IndexSpace::Table:
    return resolveIdx(Tables, Ref, ErrCode::Value::InvalidTableIdx);
  case IndexSpace::Memory:
    return resolveIdx(Memories, Ref, ErrCode::Value::InvalidMemoryIdx);
  case IndexSpace::Global:
    return resolveIdx(Globals, Ref, ErrCode::Value::InvalidGlobalIdx);
  case IndexSpace::Tag:
    return resolveIdx(Tags, Ref, ErrCode::Value::InvalidTagIdx);
  case IndexSpace::Elem:
    return resolveIdx(Elems, Ref, ErrCode::Value::InvalidElemIdx);
  case IndexSpace::Data:
    return resolveIdx(Datas, Ref, ErrCode::Value::InvalidDataIdx);
  case IndexSpace::Local:
    return resolveIdx(Locals, Ref, ErrCode::Value::InvalidLocalIdx);
  }
  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

Expect<uint32_t>
Converter::SymbolTable::resolveLabel(std::string_view Ref) const {
  if (Ref[0] == '$') {
    EXPECTED_TRY(auto Decoded, decodeIdentifier(Ref).map_error([](auto) {
      return ErrCode::Value::WatUnknownLabel;
    }));
    auto It = Labels.find(Decoded);
    if (It == Labels.end()) {
      return Unexpect(ErrCode::Value::WatUnknownLabel);
    }
    return static_cast<uint32_t>(LabelStack.size() - 1 - It->second.back());
  }
  // This is a numeric label.
  EXPECTED_TRY(auto Idx, parseUint(Ref).map_error([](auto) {
    return ErrCode::Value::WatUnknownLabel;
  }));
  if (Idx > std::numeric_limits<uint32_t>::max()) {
    return Unexpect(ErrCode::Value::WatUnknownLabel);
  }
  return static_cast<uint32_t>(Idx);
}

Expect<uint32_t>
Converter::SymbolTable::resolveField(uint32_t TypeIdx,
                                     std::string_view Ref) const {
  if (!Ref.empty() && Ref[0] == '$') {
    EXPECTED_TRY(auto Decoded, decodeIdentifier(Ref).map_error([](auto) {
      return ErrCode::Value::WatUnknownId;
    }));
    auto TIt = FieldNames.find(TypeIdx);
    if (TIt == FieldNames.end()) {
      return Unexpect(ErrCode::Value::WatUnknownId);
    }
    auto FIt = TIt->second.find(Decoded);
    if (FIt == TIt->second.end()) {
      return Unexpect(ErrCode::Value::WatUnknownId);
    }
    return FIt->second;
  }
  // This is a numeric field index. The parse is decimal, and it examines the
  // overflow, as in resolveIdx. The parser then rejects a value that is out of
  // range and does not wrap it.
  uint32_t Idx = 0;
  for (char C : Ref) {
    if (C == '_') {
      continue;
    }
    int D = (C >= '0' && C <= '9') ? C - '0' : -1;
    if (D < 0) {
      return Unexpect(ErrCode::Value::WatUnknownId);
    }
    if (Idx > std::numeric_limits<uint32_t>::max() / 10 ||
        (Idx == std::numeric_limits<uint32_t>::max() / 10 &&
         static_cast<uint32_t>(D) >
             std::numeric_limits<uint32_t>::max() % 10)) {
      return Unexpect(ErrCode::Value::WatUnknownId);
    }
    Idx = Idx * 10 + static_cast<uint32_t>(D);
  }
  return Idx;
}

std::string_view Converter::nodeText(Node N) const { return N.text(Source); }

NodeType Converter::nodeType(Node N) {
  if (N.isNull()) {
    return NodeType::Unknown;
  }
  static const std::unordered_map<std::string_view, NodeType, Hash::Hash> Map =
      {
          {"root"sv, NodeType::Sexpr},
          {"sexpr"sv, NodeType::Sexpr},
          {"keyword"sv, NodeType::Keyword},
          {"u"sv, NodeType::U},
          {"s"sv, NodeType::S},
          {"f"sv, NodeType::F},
          {"string"sv, NodeType::String},
          {"id"sv, NodeType::Id},
          {"reserved"sv, NodeType::Reserved},
          {"ERROR"sv, NodeType::Error},
      };
  auto It = Map.find(N.type());
  return It != Map.end() ? It->second : NodeType::Unknown;
}

/// Give the node type at the current position of the cursor C.
NodeType Converter::peekType(const Cursor &C) const {
  if (!C.valid()) {
    return NodeType::Unknown;
  }
  return nodeType(C.node());
}

bool Converter::sexprMatch(const Cursor &C, std::string_view KW) const {
  if (peekType(C) != NodeType::Sexpr) {
    return false;
  }
  Cursor FC(C.node());
  return peekType(FC) == NodeType::Keyword && nodeText(FC.node()) == KW;
}

bool Converter::sexprUnmatch(const Cursor &C, std::string_view KW) const {
  if (peekType(C) != NodeType::Sexpr) {
    return false;
  }
  Cursor FC(C.node());
  return peekType(FC) == NodeType::Keyword && nodeText(FC.node()) != KW;
}

ErrCode::Value Converter::classifyError(Node Root) const {
  // Descend to the first ERROR node.
  Node Err = Root;
  while (nodeType(Err) != NodeType::Error) {
    bool Found = false;
    for (uint32_t I = 0; I < Err.childCount(); ++I) {
      Node Child = Err.child(I);
      if (nodeType(Child) == NodeType::Error) {
        Err = Child;
        Found = true;
        break;
      }
      if (Child.hasError()) {
        Err = Child;
        Found = true;
        break;
      }
    }
    if (!Found) {
      return ErrCode::Value::WatUnexpectedToken;
    }
  }

  auto Text = Err.text(Source);

  // Find the annotation errors. In (@id ...), the id must come directly after
  // the @.
  auto AtPos = Text.find('@');
  if (AtPos != std::string_view::npos) {
    auto After = Text.substr(AtPos + 1);
    if (After.empty() || After.front() == ')' || After.front() == '(' ||
        After.front() == ' ' || After.front() == '\t' ||
        After.front() == '\n' || After.front() == '\r') {
      return ErrCode::Value::WatEmptyAnnotationId;
    }
    if (AtPos > 0 && Text[AtPos - 1] == '(') {
      return ErrCode::Value::WatUnclosedAnnotation;
    }
  }

  return ErrCode::Value::WatUnexpectedToken;
}

static void collectLeaves(Node N, std::vector<Node> &Leaves) {
  // Use a tree-sitter cursor, because it moves to the next sibling in O(1)
  // time. An indexed child access needs O(i) time for each call, so a simple
  // walk needs O(n^2) time on a wide tree. A deep error-recovery node makes
  // such a wide tree.
  TSNode Root;
  std::memcpy(&Root, N.Storage, sizeof(Root));
  if (ts_node_child_count(Root) == 0) {
    Leaves.push_back(N);
    return;
  }
  TSTreeCursor Cursor = ts_tree_cursor_new(Root);
  // Descend to the leftmost leaf.
  while (ts_tree_cursor_goto_first_child(&Cursor)) {
  }
  // Walk all the leaves. Move to the next sibling, then descend.
  for (;;) {
    TSNode Current = ts_tree_cursor_current_node(&Cursor);
    if (ts_node_child_count(Current) == 0 &&
        ts_node_type(Current) != "annotation"sv) {
      Node Leaf;
      std::memcpy(Leaf.Storage, &Current, sizeof(Current));
      Leaves.push_back(Leaf);
    }
    // Move to the next sibling. If there is no sibling, ascend first.
    if (ts_tree_cursor_goto_next_sibling(&Cursor)) {
      // Descend to the leftmost leaf of this sibling.
      while (ts_tree_cursor_goto_first_child(&Cursor)) {
      }
    } else {
      // Ascend. The walk is complete at the root level.
      bool Found = false;
      while (ts_tree_cursor_goto_parent(&Cursor)) {
        TSNode Parent = ts_tree_cursor_current_node(&Cursor);
        if (ts_node_eq(Parent, Root)) {
          break;
        }
        if (ts_tree_cursor_goto_next_sibling(&Cursor)) {
          while (ts_tree_cursor_goto_first_child(&Cursor)) {
          }
          Found = true;
          break;
        }
      }
      if (!Found) {
        break;
      }
    }
  }
  ts_tree_cursor_delete(&Cursor);
}

uint32_t Converter::findOrCreateFuncType(AST::FunctionType FuncTy,
                                         AST::Module &Mod) {
  auto &Types = Mod.getTypeSection().getContent();
  for (uint32_t I = 0; I < Types.size(); ++I) {
    // A type in a rec group of two types or more has a distinct identity, even
    // if the structure is equal. Use again only a standalone type or a rec
    // group of one type.
    if (auto RI = Types[I].getRecursiveInfo();
        RI.has_value() && RI->RecTypeSize > 1) {
      continue;
    }
    // An inline function-type abbreviation gives a final type with no
    // supertype. Use again only a candidate that is also final and has no
    // supertype. A definition that is not final, or that has a supertype, has
    // a different identity.
    if (Types[I].isFinal() && Types[I].getSuperTypeIndices().empty() &&
        Types[I].getCompositeType().isFunc() &&
        Types[I].getCompositeType().getFuncType() == FuncTy) {
      return I;
    }
  }
  uint32_t NewIdx = static_cast<uint32_t>(Types.size());
  Types.emplace_back(std::move(FuncTy));
  Types.back().setTypeIndex(NewIdx);
  Syms.NextType = static_cast<uint32_t>(Types.size());
  return NewIdx;
}

// module ::= ( module id? modulefield* )
// This is the main entry. It converts the parse tree into an AST::Module with
// two passes: the first pass collects the indices, and the second pass builds
// the module.
Expect<AST::Module> Converter::convert(const Tree &ParseTree,
                                       std::string_view Src) {
  Source = Src;
  Syms.clear();
  PendingExports.clear();
  HasFuncDef = false;
  HasTableDef = false;
  HasMemoryDef = false;
  HasGlobalDef = false;
  HasStart = false;

  // Scan early for illegal characters and for malformed UTF-8. Outside a
  // string and a comment, WAT does not permit a control character, DEL, or
  // non-ASCII. A comment and a string can hold any UTF-8, but the converter
  // always rejects malformed UTF-8.
  {
    auto *Data = reinterpret_cast<const unsigned char *>(Src.data());
    size_t Size = Src.size();
    bool InString = false;
    for (size_t I = 0; I < Size; ++I) {
      auto C = Data[I];
      if (InString) {
        if (C >= 0x80) {
          // In a string, valid non-ASCII UTF-8 is permitted, but malformed
          // UTF-8 is not permitted.
          auto Err = classifyNonASCII(Data, Size, I);
          if (Err == ErrCode::Value::MalformedUTF8) {
            return Unexpect(Err);
          }
          // classifyNonASCII moved I past the sequence. Adjust I for the ++I
          // of the loop.
          --I;
          continue;
        }
        if (C == '"') {
          // If the count of the backslashes before the quote is even, then
          // the quote is real and has no escape. A count of 0 is even.
          size_t Backslashes = 0;
          while (Backslashes < I && Src[I - 1 - Backslashes] == '\\') {
            ++Backslashes;
          }
          if (Backslashes % 2 == 0) {
            InString = false;
          }
        }
        continue;
      }
      // Skip a line comment. It starts with ;; and ends at the end of the
      // line.
      if (C == ';' && I + 1 < Size && Data[I + 1] == ';') {
        I += 2;
        while (I < Size && Data[I] != '\n') {
          ++I;
        }
        continue;
      }
      // Skip a block comment. It has the form (; ... ;), and it can nest.
      if (C == '(' && I + 1 < Size && Data[I + 1] == ';') {
        I += 2;
        unsigned Depth = 1;
        while (I < Size && Depth > 0) {
          if (Data[I] == '(' && I + 1 < Size && Data[I + 1] == ';') {
            ++Depth;
            ++I;
          } else if (Data[I] == ';' && I + 1 < Size && Data[I + 1] == ')') {
            --Depth;
            ++I;
          }
          ++I;
        }
        --I; // Adjust I for the ++I of the outer loop.
        continue;
      }
      if (C == '"') {
        InString = true;
        continue;
      }
      if (C >= 0x80) {
        return Unexpect(classifyNonASCII(Data, Size, I));
      }
      if (C < 0x20 && C != '\t' && C != '\n' && C != '\r') {
        return Unexpect(ErrCode::Value::WatIllegalCharacter);
      }
      if (C == 0x7F) {
        return Unexpect(ErrCode::Value::WatIllegalCharacter);
      }
    }
  }

  Node Root = ParseTree.rootNode();
  if (Root.isNull()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  {
    std::vector<Node> Leaves;
    collectLeaves(Root, Leaves);

    for (const auto &Leaf : Leaves) {
      if (!Leaf.isNamed()) {
        continue;
      }
      auto Text = Leaf.text(Source);
      auto Type = nodeType(Leaf);
      // A reserved token shows two adjacent tokens with no whitespace, or a
      // malformed numeric literal. If the tree has errors, skip a reserved
      // token that starts with '@'. Such a token comes from an annotation
      // that did not parse, and classifyError reports it.
      if (Type == NodeType::Reserved) {
        if (!Text.empty() && Text.front() == '@') {
          if (Root.hasError()) {
            continue;
          }
          auto ParentText = Leaf.parent().text(Source);
          // The token starts with '@', and the parent holds "(@". The
          // annotation scanner can fail here on an unclosed string.
          if (ParentText.find("(@"sv) != std::string_view::npos) {
            auto AtPos = ParentText.find("(@"sv);
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
    }

    // Examine the quoted annotation ids. Reject an empty (@""), a control
    // character, and malformed UTF-8. The scan for adjacent tokens finds a
    // bare @ in (@ ...).
    {
      auto Pos = Source.find("(@\""sv);
      while (Pos != std::string_view::npos) {
        size_t J = Pos + 3; // This position comes after the (@" prefix.
        if (J < Source.size() && Source[J] == '"') {
          return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
        }
        while (J < Source.size() && Source[J] != '"') {
          auto C = static_cast<unsigned char>(Source[J]);
          if (C < 0x20 || C == 0x7F) {
            return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
          }
          if (C >= 0x80) {
            // Make sure that the UTF-8 sequence is valid.
            auto *Data = reinterpret_cast<const unsigned char *>(Source.data());
            auto Len = Source.size();
            uint32_t Needed = 0;
            if ((C & 0xE0) == 0xC0) {
              Needed = 2;
            } else if ((C & 0xF0) == 0xE0) {
              Needed = 3;
            } else if ((C & 0xF8) == 0xF0) {
              Needed = 4;
            } else {
              return Unexpect(ErrCode::Value::MalformedUTF8);
            }
            if (J + Needed > Len) {
              return Unexpect(ErrCode::Value::MalformedUTF8);
            }
            for (uint32_t K = 1; K < Needed; ++K) {
              if ((Data[J + K] & 0xC0) != 0x80) {
                return Unexpect(ErrCode::Value::MalformedUTF8);
              }
            }
            J += Needed;
            continue;
          }
          if (Source[J] == '\\' && J + 1 < Source.size()) {
            // Reject a \HH hex escape that encodes a non-ASCII byte.
            int D1 = J + 2 < Source.size() ? hexDigit(Source[J + 1]) : -1;
            int D2 = J + 2 < Source.size() ? hexDigit(Source[J + 2]) : -1;
            if (D1 >= 0 && D2 >= 0) {
              unsigned Byte =
                  static_cast<unsigned>(D1) * 16 + static_cast<unsigned>(D2);
              if (Byte >= 0x80) {
                return Unexpect(ErrCode::Value::MalformedUTF8);
              }
              // Consume the two hex digits. The ++J that comes after
              // consumes the backslash.
              J += 2;
            } else {
              // Consume the single escape character after the backslash.
              ++J;
            }
          }
          ++J;
        }
        Pos = Source.find("(@\""sv, Pos + 1);
      }
    }

    // Find the bare empty annotation ids, such as (@), (@ x), and (@(. The
    // annotation scanner can consume them without an error. Examine them only
    // if the tree has no module content. Inside an annotation these patterns
    // are data, not syntax.
    {
      bool HasModuleContent = false;
      {
        Cursor RC(Root);
        while (RC.valid()) {
          if (nodeType(RC.node()) != NodeType::Error) {
            HasModuleContent = true;
            break;
          }
          RC.next();
        }
      }
      if (!HasModuleContent) {
        auto Pos = Source.find("(@"sv);
        while (Pos != std::string_view::npos) {
          size_t J = Pos + 2; // This position comes after the (@ prefix.
          if (J < Source.size()) {
            auto C = Source[J];
            if (C == ')' || C == ' ' || C == '\t' || C == '\n' || C == '\r' ||
                C == '(') {
              return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
            }
          } else {
            // The source ends directly after the (@ prefix.
            return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
          }
          Pos = Source.find("(@"sv, J);
        }
      }
    }

    // Two tokens need whitespace between them. Tree-sitter can accept two
    // adjacent tokens, for example "i32.const0" as "i32.const" and "0". The
    // WAT spec does not permit this. Skip this test if the tree already has a
    // deferred parse error that is more specific.
    if (!Root.hasError()) {
      for (size_t I = 1; I < Leaves.size(); ++I) {
        if (Leaves[I - 1].endByte() != Leaves[I].startByte()) {
          continue;
        }
        // These two leaf tokens are adjacent and have no gap. A '(' and a ')'
        // can touch any token.
        auto PrevText = Leaves[I - 1].text(Source);
        auto CurrText = Leaves[I].text(Source);
        if (PrevText == "("sv || PrevText == ")"sv || CurrText == "("sv ||
            CurrText == ")"sv) {
          continue;
        }
        // Two adjacent tokens that are not parentheses are malformed. This
        // test also rejects two adjacent strings such as "a""b". A SIMD lane
        // needs an integer. At such a position, a pattern that looks like a
        // float, for example "1" and ".5", gives an unexpected token. It does
        // not give an unknown operator.
        if ((!PrevText.empty() &&
             std::isdigit(static_cast<unsigned char>(PrevText.back()))) &&
            (!CurrText.empty() && CurrText.front() == '.')) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        if ((!PrevText.empty() && PrevText.back() == '.') &&
            (!CurrText.empty() &&
             std::isdigit(static_cast<unsigned char>(CurrText.front())))) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        return Unexpect(ErrCode::Value::WatUnknownOperator);
      }
    }
  } // This is the end of the scope of Leaves.

  // Pass 1: collect identifiers and assign indices.
  EXPECTED_TRY(collectIndices(Root));

  // Pass 2: build AST::Module.
  AST::Module Mod;
  Mod.getMagic() = {0x00, 0x61, 0x73, 0x6D};
  Mod.getVersion() = {0x01, 0x00, 0x00, 0x00};
  CurMod = &Mod;
  EXPECTED_TRY(buildModule(Root, Mod));

  // Set TypeIndex on each type. The binary loader does this when it loads the
  // module.
  {
    auto &Types = Mod.getTypeSection().getContent();
    for (uint32_t I = 0; I < Types.size(); ++I) {
      Types[I].setTypeIndex(I);
    }
  }

  // Set the DefType pointers on the tag types of the tag section and of the
  // import section. The binary loader does this in setTagFunctionType().
  {
    auto &TypeVec = Mod.getTypeSection().getContent();
    for (auto &TgType : Mod.getTagSection().getContent()) {
      auto TypeIdx = TgType.getTypeIdx();
      if (TypeIdx < TypeVec.size()) {
        TgType.setDefType(&TypeVec[TypeIdx]);
      }
    }
    for (auto &Desc : Mod.getImportSection().getContent()) {
      if (Desc.getExternalType() == ExternalType::Tag) {
        auto &TgType = Desc.getExternalTagType();
        auto TypeIdx = TgType.getTypeIdx();
        if (TypeIdx < TypeVec.size()) {
          TgType.setDefType(&TypeVec[TypeIdx]);
        }
      }
    }
  }

  // Write the deferred inline exports.
  for (const auto &Exp : PendingExports) {
    auto &Exports = Mod.getExportSection().getContent();
    Exports.emplace_back();
    Exports.back().setExternalName(Exp.Name);
    Exports.back().setExternalType(Exp.Type);
    Exports.back().setExternalIndex(Exp.Index);
  }

  // If the module has data segments, set the data count section.
  auto &DataSec = Mod.getDataSection().getContent();
  if (!DataSec.empty()) {
    Mod.getDataCountSection().setContent(static_cast<uint32_t>(DataSec.size()));
  }

  // If the conversion is successful but a parse error occurred, return the
  // parse error.
  if (Root.hasError()) {
    return Unexpect(classifyError(Root));
  }

  return Mod;
}

// Pass 1 walks the module fields and assigns an index to each named
// definition.
Expect<void> Converter::collectIndices(Node Root) {
  // Root maps to a Sexpr. It holds either a module sexpr or bare module
  // fields.
  Cursor C(Root);
  while (C.valid()) {
    Node Child = C.node();
    C.next();
    if (nodeType(Child) == NodeType::Sexpr) {
      Cursor FC(Child);
      auto KWNode = FC.node();
      if (nodeType(KWNode) != NodeType::Keyword) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      auto KW = nodeText(FC.node());
      if (KW == "module"sv) {
        // Recurse into the module's children.
        Cursor IC(Child);
        while (IC.valid()) {
          Node GC = IC.node();
          IC.next();
          if (nodeType(GC) == NodeType::Sexpr) {
            EXPECTED_TRY(collectModuleField(GC));
          }
        }
      } else {
        // Bare module field (no wrapping module sexpr).
        EXPECTED_TRY(collectModuleField(Child));
      }
    }
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
  auto KW = nodeText(FC.node());
  FC.next(); // Move FC to the position after the first keyword.

  // Extract the optional id that comes directly after the keyword. The caller
  // must first move the cursor past the keyword. On success, this lambda
  // consumes the Id node.
  auto GetId = [&](Cursor &C) -> Expect<std::string> {
    if (C.valid()) {
      auto Imm = C.node();
      if (nodeType(Imm) == NodeType::Id) {
        EXPECTED_TRY(auto Text, decodeIdentifier(nodeText(Imm)));
        C.next();
        return Text;
      }
    }
    return std::string();
  };

  auto CheckDup = [](auto &Map, const std::string &Id, uint32_t Val,
                     ErrCode::Value Err) -> Expect<void> {
    if (!Id.empty()) {
      if (!Map.emplace(Id, Val).second) {
        return Unexpect(Err);
      }
    }
    return {};
  };

  if (KW == "type"sv) {
    EXPECTED_TRY(auto Id, GetId(FC));
    EXPECTED_TRY(CheckDup(Syms.Types, Id, Syms.NextType,
                          ErrCode::Value::WatDuplicateId));
    Syms.NextType++;
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
          EXPECTED_TRY(auto Id, GetId(ChildC));
          EXPECTED_TRY(CheckDup(Syms.Types, Id, Syms.NextType,
                                ErrCode::Value::WatDuplicateId));
          Syms.NextType++;
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
      auto DescKW = nodeText(ChildC.node());
      ChildC.next(); // Move past the descriptor keyword.
      EXPECTED_TRY(auto Id, GetId(ChildC));
      decltype(Syms.Funcs) *Map = nullptr;
      uint32_t *Counter = nullptr;
      ErrCode::Value Err{};
      if (DescKW == "func"sv) {
        Map = &Syms.Funcs;
        Counter = &Syms.NextFunc;
        Err = ErrCode::Value::WatDuplicateFunc;
      } else if (DescKW == "table"sv) {
        Map = &Syms.Tables;
        Counter = &Syms.NextTable;
        Err = ErrCode::Value::WatDuplicateTable;
      } else if (DescKW == "memory"sv) {
        Map = &Syms.Memories;
        Counter = &Syms.NextMemory;
        Err = ErrCode::Value::WatDuplicateMemory;
      } else if (DescKW == "global"sv) {
        Map = &Syms.Globals;
        Counter = &Syms.NextGlobal;
        Err = ErrCode::Value::WatDuplicateGlobal;
      } else if (DescKW == "tag"sv) {
        Map = &Syms.Tags;
        Counter = &Syms.NextTag;
        Err = ErrCode::Value::WatDuplicateId;
      } else {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      EXPECTED_TRY(CheckDup(*Map, Id, *Counter, Err));
      (*Counter)++;
    }
  } else if (KW == "func"sv) {
    EXPECTED_TRY(auto Id, GetId(FC));
    EXPECTED_TRY(CheckDup(Syms.Funcs, Id, Syms.NextFunc,
                          ErrCode::Value::WatDuplicateFunc));
    Syms.NextFunc++;
  } else if (KW == "table"sv) {
    EXPECTED_TRY(auto Id, GetId(FC));
    EXPECTED_TRY(CheckDup(Syms.Tables, Id, Syms.NextTable,
                          ErrCode::Value::WatDuplicateTable));
    Syms.NextTable++;
  } else if (KW == "memory"sv) {
    EXPECTED_TRY(auto Id, GetId(FC));
    EXPECTED_TRY(CheckDup(Syms.Memories, Id, Syms.NextMemory,
                          ErrCode::Value::WatDuplicateMemory));
    Syms.NextMemory++;
  } else if (KW == "global"sv) {
    EXPECTED_TRY(auto Id, GetId(FC));
    EXPECTED_TRY(CheckDup(Syms.Globals, Id, Syms.NextGlobal,
                          ErrCode::Value::WatDuplicateGlobal));
    Syms.NextGlobal++;
  } else if (KW == "tag"sv) {
    EXPECTED_TRY(auto Id, GetId(FC));
    EXPECTED_TRY(
        CheckDup(Syms.Tags, Id, Syms.NextTag, ErrCode::Value::WatDuplicateId));
    Syms.NextTag++;
  } else if (KW == "elem"sv) {
    // The first id in `(elem $x ...)` has two meanings. In the full form it is
    // the id of the segment. In the active abbreviation
    // `(elem $tab (offset ...) ...)` it is a table reference. A lookahead is
    // necessary to tell them apart, so this code does not examine duplicates
    // here. Such a test would wrongly reject two active segments that use one
    // table.
    EXPECTED_TRY(auto Id, GetId(FC));
    if (!Id.empty()) {
      Syms.Elems[Id] = Syms.NextElem;
    }
    Syms.NextElem++;
  } else if (KW == "data"sv) {
    // The id has two meanings here, as in elem. In
    // `(data $mem (offset ...) ...)` it is a memory reference, not a segment
    // id, so this code does not examine duplicates here.
    EXPECTED_TRY(auto Id, GetId(FC));
    if (!Id.empty()) {
      Syms.Datas[Id] = Syms.NextData;
    }
    Syms.NextData++;
  }
  // The other fields, such as export and start, do not define a name. Skip
  // them.
  return {};
}

// Pass 2 converts each module field into AST sections. It converts the types
// first. Then it converts all the other fields in the order of the source.
Expect<void> Converter::buildModule(Node Root, AST::Module &Mod) {
  // Collect the module fields, which are the sexpr children.
  std::vector<Node> Fields;
  {
    Cursor C(Root);
    while (C.valid()) {
      Node Child = C.node();
      C.next();
      if (nodeType(Child) == NodeType::Sexpr) {
        Cursor FC(Child);
        if (peekType(FC) != NodeType::Keyword) {
          continue;
        }
        auto KW = nodeText(FC.node());
        if (KW == "module"sv) {
          // Descend into the children of the module.
          Cursor IC(Child);
          while (IC.valid()) {
            Node GC = IC.node();
            IC.next();
            if (nodeType(GC) == NodeType::Sexpr) {
              Fields.push_back(GC);
            }
          }
        } else {
          Fields.push_back(Child);
        }
      }
    }
  }

  // Convert the type definitions first. An explicit type must get its correct
  // index before the converter makes the implicit function types.
  for (auto &Field : Fields) {
    Cursor FC(Field);
    auto KW = peekType(FC) == NodeType::Keyword ? nodeText(FC.node()) : ""sv;
    if (KW == "type"sv || KW == "rec"sv) {
      EXPECTED_TRY(buildModuleField(Field, Mod));
    }
  }

  // Then convert all the other fields in the order of the source. The types
  // are already complete.
  for (auto &Field : Fields) {
    Cursor FC(Field);
    auto KW = peekType(FC) == NodeType::Keyword ? nodeText(FC.node()) : ""sv;
    if (KW == "type"sv || KW == "rec"sv) {
      continue;
    }
    EXPECTED_TRY(buildModuleField(Field, Mod));
  }

  return {};
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

  // A func, table, memory, or global with an inline import is an import, not
  // a definition. An example is (func (import "m" "f") ...).
  auto hasInlineImport = [this](Node N) -> bool {
    Cursor HC(N);
    while (HC.valid()) {
      Node Child = HC.node();
      HC.next();
      if (nodeType(Child) == NodeType::Sexpr) {
        Cursor ChildC(Child);
        if (peekType(ChildC) == NodeType::Keyword) {
          if (nodeText(ChildC.node()) == "import"sv) {
            return true;
          }
        }
      }
    }
    return false;
  };

  // Examine the order of the imports. Every import must come before every
  // definition.
  auto checkImportOrder = [this]() -> Expect<void> {
    if (HasFuncDef) {
      return Unexpect(ErrCode::Value::WatImportAfterFunc);
    } else if (HasTableDef) {
      return Unexpect(ErrCode::Value::WatImportAfterTable);
    } else if (HasMemoryDef) {
      return Unexpect(ErrCode::Value::WatImportAfterMemory);
    } else if (HasGlobalDef) {
      return Unexpect(ErrCode::Value::WatImportAfterGlobal);
    }
    return {};
  };

  if (KW == "type"sv) {
    return convertTypedef(Field, Mod);
  } else if (KW == "rec"sv) {
    return convertRecType(Field, Mod);
  } else if (KW == "import"sv) {
    EXPECTED_TRY(checkImportOrder());
    return convertImport(Field, Mod);
  } else if (KW == "export"sv) {
    return convertExport(Field, Mod);
  } else if (KW == "func"sv) {
    if (hasInlineImport(Field)) {
      EXPECTED_TRY(checkImportOrder());
    } else {
      HasFuncDef = true;
    }
    return convertFunc(Field, Mod);
  } else if (KW == "table"sv) {
    if (hasInlineImport(Field)) {
      EXPECTED_TRY(checkImportOrder());
    } else {
      HasTableDef = true;
    }
    return convertTable(Field, Mod);
  } else if (KW == "memory"sv) {
    if (hasInlineImport(Field)) {
      EXPECTED_TRY(checkImportOrder());
    } else {
      HasMemoryDef = true;
    }
    return convertMemory(Field, Mod);
  } else if (KW == "global"sv) {
    if (hasInlineImport(Field)) {
      EXPECTED_TRY(checkImportOrder());
    } else {
      HasGlobalDef = true;
    }
    return convertGlobal(Field, Mod);
  } else if (KW == "start"sv) {
    if (HasStart) {
      return Unexpect(ErrCode::Value::WatMultipleStart);
    }
    HasStart = true;
    return convertStart(Field, Mod);
  } else if (KW == "elem"sv) {
    return convertElem(Field, Mod);
  } else if (KW == "data"sv) {
    return convertData(Field, Mod);
  } else if (KW == "tag"sv) {
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
