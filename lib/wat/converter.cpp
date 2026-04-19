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

// Classify a non-ASCII byte sequence at Data[Pos] and advance Pos past it.
// Valid UTF-8 -> WatIllegalCharacter (legal Unicode, but non-ASCII is illegal
// in WAT outside strings); invalid bytes -> MalformedUTF8.
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
    // Lone continuation byte or invalid leading byte.
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
  // Reject overlong encodings and out-of-range codepoints.
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
  // Numeric index: decimal or 0x hex.
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
  // Bounds-check numeric type indices. NextType grows as resolveTypeUse
  // creates implicit types.
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
  // Numeric label
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
  // Numeric field index: decimal parse with overflow checking (as in
  // resolveIdx) so out-of-range is rejected instead of wrapping.
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

/// Peek the type of the current node of C
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

  // Annotation errors: (@id ...) where id must immediately follow @.
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
  // Use a tree-sitter cursor for O(1) sibling traversal; indexed child access
  // is O(i) per call, making naive traversal O(n^2) on wide trees (e.g. deeply
  // nested error-recovery nodes).
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
  // Walk all leaves via next-sibling + descend.
  for (;;) {
    TSNode Current = ts_tree_cursor_current_node(&Cursor);
    if (ts_node_child_count(Current) == 0 &&
        ts_node_type(Current) != "annotation"sv) {
      Node Leaf;
      std::memcpy(Leaf.Storage, &Current, sizeof(Current));
      Leaves.push_back(Leaf);
    }
    // Go to next sibling, or ascend until we can.
    if (ts_tree_cursor_goto_next_sibling(&Cursor)) {
      // Descend to leftmost leaf of this sibling.
      while (ts_tree_cursor_goto_first_child(&Cursor)) {
      }
    } else {
      // Ascend; reaching the root level means we're done.
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
    // Types in multi-type rec groups have a distinct identity even when the
    // structure matches; only reuse standalone or single-type rec groups.
    if (auto RI = Types[I].getRecursiveInfo();
        RI.has_value() && RI->RecTypeSize > 1) {
      continue;
    }
    // An inline function-type abbreviation denotes a final, supertype-less
    // type, so only reuse a candidate that is likewise final and supertype-less
    // — a non-final or sub-typed definition has a different identity.
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
// Main entry: parse tree -> AST::Module in two passes (collect indices, build).
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

  // Early scan for illegal characters and malformed UTF-8. Outside strings and
  // comments, control chars, DEL, and non-ASCII are illegal. Comments and
  // strings may hold arbitrary UTF-8, but malformed UTF-8 is always rejected.
  {
    auto *Data = reinterpret_cast<const unsigned char *>(Src.data());
    size_t Size = Src.size();
    bool InString = false;
    for (size_t I = 0; I < Size; ++I) {
      auto C = Data[I];
      if (InString) {
        if (C >= 0x80) {
          // In strings, valid non-ASCII UTF-8 is fine but malformed is not.
          auto Err = classifyNonASCII(Data, Size, I);
          if (Err == ErrCode::Value::MalformedUTF8) {
            return Unexpect(Err);
          }
          // classifyNonASCII advanced I past the sequence; adjust for ++I.
          --I;
          continue;
        }
        if (C == '"') {
          // An even count of preceding backslashes (including 0) means the
          // quote is real (not escaped).
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
      // Skip line comments: ;; to end of line.
      if (C == ';' && I + 1 < Size && Data[I + 1] == ';') {
        I += 2;
        while (I < Size && Data[I] != '\n') {
          ++I;
        }
        continue;
      }
      // Skip block comments: (; ... ;) with nesting.
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
        --I; // adjust for outer ++I
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
      // A reserved token means adjacent tokens without whitespace or a
      // malformed numeric literal. When the tree has errors, skip '@'-prefixed
      // reserved tokens (failed annotation parsing, handled by classifyError).
      if (Type == NodeType::Reserved) {
        if (!Text.empty() && Text.front() == '@') {
          if (Root.hasError()) {
            continue;
          }
          auto ParentText = Leaf.parent().text(Source);
          // @-prefixed reserved with "(@" in the parent: the annotation scanner
          // may have failed on an unclosed string.
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
        // Bare '"' or unclosed string literal.
        if (!Text.empty() && Text.front() == '"') {
          auto Close = Text.find('"', 1);
          if (Close == std::string_view::npos) {
            return Unexpect(ErrCode::Value::WatUnclosedString);
          }
        }
        return Unexpect(ErrCode::Value::WatUnknownOperator);
      }
    }

    // Quoted annotation ids: reject empty (@"") and control chars / malformed
    // UTF-8 inside. (Bare @ in (@ ...) is caught by the adjacency scan above.)
    {
      auto Pos = Source.find("(@\""sv);
      while (Pos != std::string_view::npos) {
        size_t J = Pos + 3; // after (@"
        if (J < Source.size() && Source[J] == '"') {
          return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
        }
        while (J < Source.size() && Source[J] != '"') {
          auto C = static_cast<unsigned char>(Source[J]);
          if (C < 0x20 || C == 0x7F) {
            return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
          }
          if (C >= 0x80) {
            // Validate UTF-8 sequence.
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
            // Reject \HH hex escapes encoding non-ASCII bytes.
            int D1 = J + 2 < Source.size() ? hexDigit(Source[J + 1]) : -1;
            int D2 = J + 2 < Source.size() ? hexDigit(Source[J + 2]) : -1;
            if (D1 >= 0 && D2 >= 0) {
              unsigned Byte =
                  static_cast<unsigned>(D1) * 16 + static_cast<unsigned>(D2);
              if (Byte >= 0x80) {
                return Unexpect(ErrCode::Value::MalformedUTF8);
              }
              // Consume the two hex digits (backslash consumed by ++J below).
              J += 2;
            } else {
              // Consume the single escape char after the backslash.
              ++J;
            }
          }
          ++J;
        }
        Pos = Source.find("(@\""sv, Pos + 1);
      }
    }

    // Bare empty annotation ids: (@), (@ x), (@( etc. The annotation scanner
    // may consume these without errors. Only check when the tree has no module
    // content — inside annotations these patterns are just data, not syntax.
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
          size_t J = Pos + 2; // after (@
          if (J < Source.size()) {
            auto C = Source[J];
            if (C == ')' || C == ' ' || C == '\t' || C == '\n' || C == '\r' ||
                C == '(') {
              return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
            }
          } else {
            // (@  at end of source
            return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
          }
          Pos = Source.find("(@"sv, J);
        }
      }
    }

    // Require whitespace between tokens. Tree-sitter may accept adjacent tokens
    // (e.g. "i32.const0" as "i32.const" + "0"), but the WAT spec forbids it.
    // Skip when the tree already has a more-specific deferred parse error.
    if (!Root.hasError()) {
      for (size_t I = 1; I < Leaves.size(); ++I) {
        if (Leaves[I - 1].endByte() != Leaves[I].startByte()) {
          continue;
        }
        // Two adjacent leaf tokens with no gap; '(' and ')' may abut anything.
        auto PrevText = Leaves[I - 1].text(Source);
        auto CurrText = Leaves[I].text(Source);
        if (PrevText == "("sv || PrevText == ")"sv || CurrText == "("sv ||
            CurrText == ")"sv) {
          continue;
        }
        // Two adjacent non-paren tokens are malformed (this also correctly
        // rejects adjacent strings like "a""b"). A float-like pattern (e.g.
        // "1"+".5") where an integer is expected (SIMD lane) is "unexpected
        // token" rather than "unknown operator".
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
  } // Leaves scope

  // Pass 1: collect identifiers and assign indices.
  EXPECTED_TRY(collectIndices(Root));

  // Pass 2: build AST::Module.
  AST::Module Mod;
  Mod.getMagic() = {0x00, 0x61, 0x73, 0x6D};
  Mod.getVersion() = {0x01, 0x00, 0x00, 0x00};
  CurMod = &Mod;
  EXPECTED_TRY(buildModule(Root, Mod));

  // Set TypeIndex on each type (the binary loader does this during loading).
  {
    auto &Types = Mod.getTypeSection().getContent();
    for (uint32_t I = 0; I < Types.size(); ++I) {
      Types[I].setTypeIndex(I);
    }
  }

  // Set DefType pointers on tag types (tag and import sections); the binary
  // loader does this in setTagFunctionType().
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

  // Emit deferred inline exports
  for (const auto &Exp : PendingExports) {
    auto &Exports = Mod.getExportSection().getContent();
    Exports.emplace_back();
    Exports.back().setExternalName(Exp.Name);
    Exports.back().setExternalType(Exp.Type);
    Exports.back().setExternalIndex(Exp.Index);
  }

  // Set the data count section if data segments exist.
  auto &DataSec = Mod.getDataSection().getContent();
  if (!DataSec.empty()) {
    Mod.getDataCountSection().setContent(static_cast<uint32_t>(DataSec.size()));
  }

  // If conversion succeeded but there was a parse error, return it.
  if (Root.hasError()) {
    return Unexpect(classifyError(Root));
  }

  return Mod;
}

// Pass 1: walk module fields to assign indices for all named definitions.
Expect<void> Converter::collectIndices(Node Root) {
  // Root maps to Sexpr and either wraps a module sexpr or is bare module
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
// Register identifiers and assign index-space positions for one field.
Expect<void> Converter::collectModuleField(Node Field) {
  // Field should be a sexpr with a leading keyword.
  Cursor FC(Field);
  auto KWNode = FC.node();
  if (nodeType(KWNode) != NodeType::Keyword) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  auto KW = nodeText(FC.node());
  FC.next(); // Position FC after the leading keyword.

  // Extract the optional id immediately following the keyword (caller must
  // have advanced past the keyword); consumes the Id node on success.
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
    // Recursive type: each child (type ...) sexpr gets an index.
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
    // Imports consume indices in their respective spaces; find the inner
    // descriptor sexpr (func/table/memory/global/tag).
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
      ChildC.next(); // past the descriptor keyword
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
    // The leading id in `(elem $x ...)` is overloaded: the segment's own id in
    // the full form, but a table reference in the active abbreviation
    // `(elem $tab (offset ...) ...)`. Disambiguating needs lookahead, so no
    // duplicate-check here (it would wrongly reject active segments reusing a
    // table).
    EXPECTED_TRY(auto Id, GetId(FC));
    if (!Id.empty()) {
      Syms.Elems[Id] = Syms.NextElem;
    }
    Syms.NextElem++;
  } else if (KW == "data"sv) {
    // Same overloaded-id caveat as elem: `(data $mem (offset ...) ...)` is a
    // memory reference, not a segment id, so no duplicate-check here.
    EXPECTED_TRY(auto Id, GetId(FC));
    if (!Id.empty()) {
      Syms.Datas[Id] = Syms.NextData;
    }
    Syms.NextData++;
  }
  // export, start, etc. don't define names — skip.
  return {};
}

// Pass 2: convert each module field into AST sections. Types are processed
// first, then all other fields in source order.
Expect<void> Converter::buildModule(Node Root, AST::Module &Mod) {
  // Collect module fields (sexpr children).
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
          // Recurse into module children.
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

  // Process type definitions first so that explicit types get their correct
  // indices before implicit function types are created.
  for (auto &Field : Fields) {
    Cursor FC(Field);
    auto KW = peekType(FC) == NodeType::Keyword ? nodeText(FC.node()) : ""sv;
    if (KW == "type"sv || KW == "rec"sv) {
      EXPECTED_TRY(buildModuleField(Field, Mod));
    }
  }

  // Then all other fields in source order (types already done).
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

// Dispatch a single module field to the appropriate converter by keyword.
Expect<void> Converter::buildModuleField(Node Field, AST::Module &Mod) {
  Cursor FC(Field);
  if (peekType(FC) != NodeType::Keyword) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  auto KW = nodeText(FC.node());
  FC.next();

  // A func/table/memory/global with an inline import, e.g.
  // (func (import "m" "f") ...), is an import, not a definition.
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

  // Check import ordering: ALL imports must come before ANY definitions.
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
  // Any other keyword is an unrecognized module field; reject rather than
  // silently drop it (e.g. `(module (bogus))` must not parse as valid).
  // Annotations are tree-sitter extras and never reach here; a non-keyword
  // first token was rejected above.
  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

} // namespace WAT
} // namespace WasmEdge
