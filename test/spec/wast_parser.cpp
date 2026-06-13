// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "wast_parser.h"
#include "common/hash.h"
#include "common/spdlog.h"
#include "wat/parser.h"
#include "wat/wat_util.h"

#include "tree_sitter.h"

extern "C" const TSLanguage *tree_sitter_wat();

#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <unordered_map>

namespace WasmEdge {
namespace Wast {

namespace {
using namespace WasmEdge::WAT; // Node, Cursor, Tree, Parser
using namespace std::literals;

/// Node type enum for the WAT grammar.
/// Mirrors the enum in converter.h — defined locally to avoid depending
/// on WAT converter internals.
enum class NodeType {
  Sexpr,
  Keyword,
  U,
  S,
  F,
  String,
  Id,
  Error,
  Unknown,
};

enum class ConstKind {
  I32,
  I64,
  F32,
  F64,
  V128,
  RefNull,
  RefExtern,
  RefHost,
  RefFunc,
};

enum class ResultKind {
  I32,
  I64,
  F32,
  F64,
  V128,
  RefNull,
  RefExtern,
  RefHost,
  RefFunc,
  RefAny,
  RefEq,
  RefI31,
  RefStruct,
  RefArray,
  RefExn,
};

enum class CmdKind {
  Module,
  Register,
  Invoke,
  Get,
  AssertReturn,
  AssertTrap,
  AssertExhaustion,
  AssertInvalid,
  AssertMalformed,
  AssertUnlinkable,
  AssertUninstantiable,
  AssertException,
  Thread,
  Wait,
};

// --- WastConverter ---
// Walks a WAT grammar parse tree (generic sexpr/keyword nodes) and produces
// a WastScript. Follows the same pattern as the WAT Converter class:
// all semantic interpretation via keyword-string matching.

class WastConverter {
public:
  Expect<WastScript> convert(const Tree &T, std::string Source);

private:
  // --- Helpers (same pattern as WAT Converter) ---

  std::string_view nodeText(Node N) const { return N.text(Source_); }

  NodeType nodeType(Node N) const {
    if (N.isNull()) {
      return NodeType::Unknown;
    }
    auto T = N.type();
    if (T == "sexpr"sv || T == "root"sv)
      return NodeType::Sexpr;
    if (T == "keyword"sv)
      return NodeType::Keyword;
    if (T == "u"sv)
      return NodeType::U;
    if (T == "s"sv)
      return NodeType::S;
    if (T == "f"sv)
      return NodeType::F;
    if (T == "string"sv)
      return NodeType::String;
    if (T == "id"sv)
      return NodeType::Id;
    if (T == "ERROR"sv)
      return NodeType::Error;
    return NodeType::Unknown;
  }

  /// Check if a node is a numeric token (u, s, or f).
  bool isNumeric(Node N) const {
    auto T = nodeType(N);
    return T == NodeType::U || T == NodeType::S || T == NodeType::F;
  }

  /// Check if a node is a NaN pattern token (nan:canonical or nan:arithmetic).
  Result::NaNPattern nanPattern(Node N) const {
    if (nodeType(N) != NodeType::Keyword) {
      return Result::NaNPattern::None;
    }
    auto Text = nodeText(N);
    if (Text == "nan:canonical"sv)
      return Result::NaNPattern::Canonical;
    if (Text == "nan:arithmetic"sv)
      return Result::NaNPattern::Arithmetic;
    return Result::NaNPattern::None;
  }

  /// Return the keyword text of the first child of an sexpr node, or an
  /// empty string_view if the first child is not a keyword.
  std::string_view firstKeyword(Node N) const {
    Cursor C(N);
    Node K = C.node();
    return nodeType(K) == NodeType::Keyword ? nodeText(K) : std::string_view{};
  }

  // --- Heap type keyword -> TypeCode ---

  TypeCode heapTypeToCode(std::string_view HT) const {
    static const std::unordered_map<std::string_view, TypeCode, Hash::Hash>
        Map = {
            {"func"sv, TypeCode::FuncRef},
            {"extern"sv, TypeCode::ExternRef},
            {"any"sv, TypeCode::AnyRef},
            {"eq"sv, TypeCode::EqRef},
            {"i31"sv, TypeCode::I31Ref},
            {"struct"sv, TypeCode::StructRef},
            {"array"sv, TypeCode::ArrayRef},
            {"none"sv, TypeCode::NullRef},
            {"noextern"sv, TypeCode::NullExternRef},
            {"nofunc"sv, TypeCode::NullFuncRef},
            {"exn"sv, TypeCode::ExnRef},
            {"noexn"sv, TypeCode::NullExnRef},
        };
    if (auto It = Map.find(HT); It != Map.end()) {
      return It->second;
    }
    return TypeCode::FuncRef; // default, matches prior fall-through
  }

  // --- Const expression parsing (invoke args) ---

  template <typename T, TypeCode TC>
  void parseConstExprNumeric(Cursor &C, std::vector<ValVariant> &Args,
                             std::vector<ValType> &Types,
                             T (*ParseFn)(std::string_view)) {
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (isNumeric(Child)) {
        Args.emplace_back((*ParseFn)(nodeText(Child)));
        Types.emplace_back(TC);
      }
    }
  }

  void parseConstExprV128(Cursor &C, std::vector<ValVariant> &Args,
                          std::vector<ValType> &Types) {
    uint128_t V128{};
    std::string Shape;
    std::vector<std::string_view> Nums;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      if (CT == NodeType::Keyword) {
        Shape = std::string(nodeText(Child));
      } else if (isNumeric(Child)) {
        Nums.push_back(nodeText(Child));
      }
    }
    parseV128Lanes(Shape, Nums, V128);
    Args.emplace_back(V128);
    Types.emplace_back(TypeCode::V128);
  }

  void parseConstExprRefNull(Cursor &C, std::vector<ValVariant> &Args,
                             std::vector<ValType> &Types) {
    TypeCode Code = TypeCode::FuncRef;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (nodeType(Child) == NodeType::Keyword) {
        Code = heapTypeToCode(nodeText(Child));
      }
    }
    Args.emplace_back(RefVariant(Code));
    Types.emplace_back(Code);
  }

  void parseConstExprRefOpaque(Cursor &C, std::vector<ValVariant> &Args,
                               std::vector<ValType> &Types, TypeCode Code) {
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (isNumeric(Child)) {
        auto Val = parseU32(nodeText(Child));
        Args.emplace_back(
            RefVariant(Code, reinterpret_cast<void *>(
                                 static_cast<uintptr_t>(Val) + 0x100000000ULL)));
        Types.emplace_back(Code);
      }
    }
  }

  void parseConstExprRefFunc(Cursor &, std::vector<ValVariant> &Args,
                             std::vector<ValType> &Types) {
    Args.emplace_back(RefVariant(TypeCode::FuncRef));
    Types.emplace_back(TypeCode::FuncRef);
  }

  void parseConstExpr(Node N, std::vector<ValVariant> &Args,
                      std::vector<ValType> &Types) {
    Cursor C(N);
    Node KWNode = C.node();
    if (nodeType(KWNode) != NodeType::Keyword) {
      return;
    }
    auto KW = nodeText(KWNode);
    C.next();

    static const std::unordered_map<std::string_view, ConstKind, Hash::Hash>
        Map = {
            {"i32.const"sv, ConstKind::I32},
            {"i64.const"sv, ConstKind::I64},
            {"f32.const"sv, ConstKind::F32},
            {"f64.const"sv, ConstKind::F64},
            {"v128.const"sv, ConstKind::V128},
            {"ref.null"sv, ConstKind::RefNull},
            {"ref.extern"sv, ConstKind::RefExtern},
            {"ref.host"sv, ConstKind::RefHost},
            {"ref.func"sv, ConstKind::RefFunc},
        };
    auto It = Map.find(KW);
    if (It == Map.end()) {
      return;
    }
    switch (It->second) {
    case ConstKind::I32:
      parseConstExprNumeric<uint32_t, TypeCode::I32>(C, Args, Types, &parseU32);
      break;
    case ConstKind::I64:
      parseConstExprNumeric<uint64_t, TypeCode::I64>(C, Args, Types, &parseU64);
      break;
    case ConstKind::F32:
      parseConstExprNumeric<uint32_t, TypeCode::F32>(C, Args, Types,
                                                    &parseF32Bits);
      break;
    case ConstKind::F64:
      parseConstExprNumeric<uint64_t, TypeCode::F64>(C, Args, Types,
                                                    &parseF64Bits);
      break;
    case ConstKind::V128:
      parseConstExprV128(C, Args, Types);
      break;
    case ConstKind::RefNull:
      parseConstExprRefNull(C, Args, Types);
      break;
    case ConstKind::RefExtern:
      parseConstExprRefOpaque(C, Args, Types, TypeCode::ExternRef);
      break;
    case ConstKind::RefHost:
      parseConstExprRefOpaque(C, Args, Types, TypeCode::AnyRef);
      break;
    case ConstKind::RefFunc:
      parseConstExprRefFunc(C, Args, Types);
      break;
    }
  }

  // --- V128 lane parsing helper ---

  void parseV128Lanes(const std::string &Shape,
                      const std::vector<std::string_view> &Nums,
                      uint128_t &V128) {
    if (Shape == "i8x16" && Nums.size() == 16) {
      uint8_t Lanes[16];
      for (size_t K = 0; K < 16; ++K)
        Lanes[K] = static_cast<uint8_t>(parseU32(Nums[K]));
      std::memcpy(&V128, Lanes, 16);
    } else if (Shape == "i16x8" && Nums.size() == 8) {
      uint16_t Lanes[8];
      for (size_t K = 0; K < 8; ++K)
        Lanes[K] = static_cast<uint16_t>(parseU32(Nums[K]));
      std::memcpy(&V128, Lanes, 16);
    } else if (Shape == "i32x4" && Nums.size() == 4) {
      uint32_t Lanes[4];
      for (size_t K = 0; K < 4; ++K)
        Lanes[K] = parseU32(Nums[K]);
      std::memcpy(&V128, Lanes, 16);
    } else if (Shape == "i64x2" && Nums.size() == 2) {
      uint64_t Lanes[2];
      for (size_t K = 0; K < 2; ++K)
        Lanes[K] = parseU64(Nums[K]);
      std::memcpy(&V128, Lanes, 16);
    } else if (Shape == "f32x4" && Nums.size() == 4) {
      uint32_t Lanes[4];
      for (size_t K = 0; K < 4; ++K)
        Lanes[K] = parseF32Bits(Nums[K]);
      std::memcpy(&V128, Lanes, 16);
    } else if (Shape == "f64x2" && Nums.size() == 2) {
      uint64_t Lanes[2];
      for (size_t K = 0; K < 2; ++K)
        Lanes[K] = parseF64Bits(Nums[K]);
      std::memcpy(&V128, Lanes, 16);
    }
  }

  // --- Action parsing (invoke/get) ---

  Action parseAction(Node N) {
    Action Act;
    Act.Type = ActionType::Get;
    Cursor C(N);
    Node KWNode = C.node();
    if (nodeType(KWNode) == NodeType::Keyword) {
      if (nodeText(KWNode) == "invoke"sv) {
        Act.Type = ActionType::Invoke;
      }
      C.next();
    }
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      if (CT == NodeType::Id) {
        Act.ModuleName = parseIdentifier(nodeText(Child));
      } else if (CT == NodeType::String) {
        if (auto S = parseString(nodeText(Child), false)) {
          Act.FieldName = std::move(*S);
        }
      } else if (CT == NodeType::Sexpr) {
        parseConstExpr(Child, Act.Args, Act.ArgTypes);
      }
    }
    return Act;
  }

  // --- Result parsing ---

  template <typename T>
  void parseIntResult(Cursor &C, Result &R, T (*ParseFn)(std::string_view)) {
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (isNumeric(Child)) {
        R.Value = (*ParseFn)(nodeText(Child));
      }
    }
  }

  Result parseResultI32(Cursor &C) {
    Result R;
    R.Type = ValType(TypeCode::I32);
    parseIntResult<uint32_t>(C, R, &parseU32);
    return R;
  }

  Result parseResultI64(Cursor &C) {
    Result R;
    R.Type = ValType(TypeCode::I64);
    parseIntResult<uint64_t>(C, R, &parseU64);
    return R;
  }

  template <typename T>
  void parseFloatResult(Cursor &C, Result &R, T (*ParseFn)(std::string_view)) {
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (isNumeric(Child)) {
        R.Value = (*ParseFn)(nodeText(Child));
      } else if (auto NaN = nanPattern(Child);
                 NaN != Result::NaNPattern::None) {
        R.NaN = NaN;
        R.Value = T(0);
      }
    }
  }

  Result parseResultF32(Cursor &C) {
    Result R;
    R.Type = ValType(TypeCode::F32);
    parseFloatResult<uint32_t>(C, R, &parseF32Bits);
    return R;
  }

  Result parseResultF64(Cursor &C) {
    Result R;
    R.Type = ValType(TypeCode::F64);
    parseFloatResult<uint64_t>(C, R, &parseF64Bits);
    return R;
  }

  Result parseResultV128(Cursor &C) {
    Result R;
    R.Type = ValType(TypeCode::V128);
    uint128_t V128{};
    std::string Shape;
    struct LaneEntry {
      std::string_view Text;
      Result::NaNPattern NaN = Result::NaNPattern::None;
    };
    std::vector<LaneEntry> Lanes;

    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      if (auto NaN = nanPattern(Child); NaN != Result::NaNPattern::None) {
        Lanes.push_back({{}, NaN});
      } else if (CT == NodeType::Keyword) {
        Shape = std::string(nodeText(Child));
      } else if (isNumeric(Child)) {
        Lanes.push_back({nodeText(Child), Result::NaNPattern::None});
      }
    }

    R.V128Shape = Shape;
    for (const auto &L : Lanes) {
      R.V128LaneNaN.push_back(L.NaN);
    }

    uint8_t *Bytes = reinterpret_cast<uint8_t *>(&V128);
    if (Shape == "i8x16" && Lanes.size() == 16) {
      for (size_t K = 0; K < 16; ++K)
        if (Lanes[K].NaN == Result::NaNPattern::None)
          Bytes[K] = static_cast<uint8_t>(parseU32(Lanes[K].Text));
    } else if (Shape == "i16x8" && Lanes.size() == 8) {
      for (size_t K = 0; K < 8; ++K)
        if (Lanes[K].NaN == Result::NaNPattern::None) {
          uint16_t V = static_cast<uint16_t>(parseU32(Lanes[K].Text));
          std::memcpy(Bytes + K * 2, &V, 2);
        }
    } else if (Shape == "i32x4" && Lanes.size() == 4) {
      for (size_t K = 0; K < 4; ++K)
        if (Lanes[K].NaN == Result::NaNPattern::None) {
          uint32_t V = parseU32(Lanes[K].Text);
          std::memcpy(Bytes + K * 4, &V, 4);
        }
    } else if (Shape == "i64x2" && Lanes.size() == 2) {
      for (size_t K = 0; K < 2; ++K)
        if (Lanes[K].NaN == Result::NaNPattern::None) {
          uint64_t V = parseU64(Lanes[K].Text);
          std::memcpy(Bytes + K * 8, &V, 8);
        }
    } else if (Shape == "f32x4" && Lanes.size() == 4) {
      for (size_t K = 0; K < 4; ++K)
        if (Lanes[K].NaN == Result::NaNPattern::None) {
          uint32_t V = parseF32Bits(Lanes[K].Text);
          std::memcpy(Bytes + K * 4, &V, 4);
        }
    } else if (Shape == "f64x2" && Lanes.size() == 2) {
      for (size_t K = 0; K < 2; ++K)
        if (Lanes[K].NaN == Result::NaNPattern::None) {
          uint64_t V = parseF64Bits(Lanes[K].Text);
          std::memcpy(Bytes + K * 8, &V, 8);
        }
    }
    R.Value = V128;
    return R;
  }

  Result parseResultRefNull(Cursor &C) {
    Result R;
    TypeCode Code = TypeCode::FuncRef;
    bool HasHeapType = false;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      if (CT == NodeType::Keyword) {
        HasHeapType = true;
        Code = heapTypeToCode(nodeText(Child));
      } else if (isNumeric(Child)) {
        HasHeapType = true;
      }
    }
    if (!HasHeapType) {
      Code = TypeCode::AnyRef;
      R.AnyNullRef = true;
    }
    R.Type = ValType(Code);
    R.Value = RefVariant(Code);
    return R;
  }

  Result parseResultRefOpaque(Cursor &C, TypeCode Code) {
    bool HasVal = false;
    Result R;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (isNumeric(Child)) {
        HasVal = true;
        auto Val = parseU32(nodeText(Child));
        R.Value = RefVariant(
            Code, reinterpret_cast<void *>(static_cast<uintptr_t>(Val) +
                                           0x100000000ULL));
      }
    }
    if (!HasVal) {
      return parseResultOpaqueRef(Code);
    }
    R.Type = ValType(Code);
    return R;
  }

  Result parseResultOpaqueRef(TypeCode Code) {
    Result R;
    R.Type = ValType(Code);
    R.Value = RefVariant(Code);
    R.OpaqueRef = true;
    return R;
  }

  Result parseResultConst(Node N) {
    Cursor C(N);
    Node KWNode = C.node();
    if (nodeType(KWNode) != NodeType::Keyword) {
      return Result{};
    }
    auto KW = nodeText(KWNode);
    C.next();

    static const std::unordered_map<std::string_view, ResultKind, Hash::Hash>
        Map = {
            {"i32.const"sv, ResultKind::I32},
            {"i64.const"sv, ResultKind::I64},
            {"f32.const"sv, ResultKind::F32},
            {"f64.const"sv, ResultKind::F64},
            {"v128.const"sv, ResultKind::V128},
            {"ref.null"sv, ResultKind::RefNull},
            {"ref.extern"sv, ResultKind::RefExtern},
            {"ref.host"sv, ResultKind::RefHost},
            {"ref.func"sv, ResultKind::RefFunc},
            {"ref.any"sv, ResultKind::RefAny},
            {"ref.eq"sv, ResultKind::RefEq},
            {"ref.i31"sv, ResultKind::RefI31},
            {"ref.struct"sv, ResultKind::RefStruct},
            {"ref.array"sv, ResultKind::RefArray},
            {"ref.exn"sv, ResultKind::RefExn},
        };
    auto It = Map.find(KW);
    if (It == Map.end()) {
      return Result{};
    }
    switch (It->second) {
    case ResultKind::I32:
      return parseResultI32(C);
    case ResultKind::I64:
      return parseResultI64(C);
    case ResultKind::F32:
      return parseResultF32(C);
    case ResultKind::F64:
      return parseResultF64(C);
    case ResultKind::V128:
      return parseResultV128(C);
    case ResultKind::RefNull:
      return parseResultRefNull(C);
    case ResultKind::RefExtern:
      return parseResultRefOpaque(C, TypeCode::ExternRef);
    case ResultKind::RefHost:
      return parseResultRefOpaque(C, TypeCode::AnyRef);
    case ResultKind::RefFunc:
      return parseResultOpaqueRef(TypeCode::FuncRef);
    case ResultKind::RefAny:
      return parseResultOpaqueRef(TypeCode::AnyRef);
    case ResultKind::RefEq:
      return parseResultOpaqueRef(TypeCode::EqRef);
    case ResultKind::RefI31:
      return parseResultOpaqueRef(TypeCode::I31Ref);
    case ResultKind::RefStruct:
      return parseResultOpaqueRef(TypeCode::StructRef);
    case ResultKind::RefArray:
      return parseResultOpaqueRef(TypeCode::ArrayRef);
    case ResultKind::RefExn:
      return parseResultOpaqueRef(TypeCode::ExnRef);
    }
    return Result{};
  }

  // --- Command converters ---

  ScriptCommand convertModule(Node N, Cursor &C) {
    ScriptCommand Cmd;
    Cmd.Type = CommandType::Module;
    Cmd.Line = N.startRow() + 1;
    Cmd.ModType = ModuleType::Text;
    Cmd.ModuleSource = nodeText(N);

    bool IsInstance = false;
    bool IsBinary = false;
    bool IsQuote = false;
    std::vector<std::string_view> Identifiers;

    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      auto Text = nodeText(Child);

      if (CT == NodeType::Id) {
        Identifiers.push_back(parseIdentifier(Text));
      } else if (CT == NodeType::Keyword) {
        if (Text == "binary"sv) {
          IsBinary = true;
        } else if (Text == "quote"sv) {
          IsQuote = true;
        } else if (Text == "definition"sv) {
          Cmd.Type = CommandType::ModuleDefinition;
        } else if (Text == "instance"sv) {
          IsInstance = true;
          Cmd.Type = CommandType::ModuleInstance;
        }
      }
    }

    if (IsBinary) {
      Cmd.ModType = ModuleType::Binary;
    } else if (IsQuote) {
      Cmd.ModType = ModuleType::Quote;
    }

    if (IsInstance) {
      if (Identifiers.size() >= 2) {
        Cmd.ModuleName = Identifiers[0];
        Cmd.DefinitionName = Identifiers[1];
      } else if (Identifiers.size() == 1) {
        Cmd.ModuleName = Identifiers[0];
      }
    } else if (!Identifiers.empty()) {
      Cmd.ModuleName = Identifiers[0];
    }

    return Cmd;
  }

  ScriptCommand convertBareModule(Node N) {
    ScriptCommand Cmd;
    Cmd.Type = CommandType::Module;
    Cmd.Line = N.startRow() + 1;
    Cmd.ModType = ModuleType::Text;
    Cmd.ModuleSource = nodeText(N);
    return Cmd;
  }

  ScriptCommand convertRegister(Node N, Cursor &C) {
    ScriptCommand Cmd;
    Cmd.Type = CommandType::Register;
    Cmd.Line = N.startRow() + 1;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      if (CT == NodeType::String) {
        Cmd.RegisterName = stripQuotes(nodeText(Child));
      } else if (CT == NodeType::Id) {
        Cmd.ModuleName = parseIdentifier(nodeText(Child));
      }
    }
    return Cmd;
  }

  ScriptCommand convertActionCommand(Node N, Cursor &, ActionType T) {
    ScriptCommand Cmd;
    Cmd.Type = CommandType::Action;
    Cmd.Line = N.startRow() + 1;
    Cmd.Act = parseAction(N);
    Cmd.Act->Type = T; // override — parseAction may have guessed from keyword
    return Cmd;
  }

  ScriptCommand convertAssertReturn(Node N, Cursor &C) {
    ScriptCommand Cmd;
    Cmd.Type = CommandType::AssertReturn;
    Cmd.Line = N.startRow() + 1;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (nodeType(Child) != NodeType::Sexpr)
        continue;

      auto CKW = firstKeyword(Child);

      if (CKW == "invoke"sv || CKW == "get"sv) {
        Cmd.Act = parseAction(Child);
      } else if (CKW == "either"sv) {
        ResultOrEither ROE;
        for (Cursor EC(Child); EC.valid(); EC.next()) {
          Node EChild = EC.node();
          if (nodeType(EChild) == NodeType::Sexpr) {
            ROE.Alternatives.push_back(parseResultConst(EChild));
          }
        }
        Cmd.Expected.push_back(std::move(ROE));
      } else {
        ResultOrEither ROE;
        ROE.Alternatives.push_back(parseResultConst(Child));
        Cmd.Expected.push_back(std::move(ROE));
      }
    }
    return Cmd;
  }

  ScriptCommand convertAssertTrap(Node N, Cursor &C) {
    ScriptCommand Cmd;
    Cmd.Type = CommandType::AssertTrap;
    Cmd.Line = N.startRow() + 1;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      if (CT == NodeType::Sexpr) {
        auto CKW = firstKeyword(Child);
        if (CKW == "invoke"sv || CKW == "get"sv) {
          Cmd.Act = parseAction(Child);
        } else if (CKW == "module"sv) {
          Cmd.ModType = detectModuleType(Child);
          Cmd.ModuleSource = nodeText(Child);
        } else {
          Cmd.ModType = ModuleType::Text;
          Cmd.ModuleSource = nodeText(Child);
        }
      } else if (CT == NodeType::String) {
        Cmd.ExpectedMessage = stripQuotes(nodeText(Child));
      }
    }
    return Cmd;
  }

  ScriptCommand convertAssertExhaustion(Node N, Cursor &C) {
    ScriptCommand Cmd;
    Cmd.Type = CommandType::AssertExhaustion;
    Cmd.Line = N.startRow() + 1;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      if (CT == NodeType::Sexpr) {
        auto CKW = firstKeyword(Child);
        if (CKW == "invoke"sv || CKW == "get"sv) {
          Cmd.Act = parseAction(Child);
        }
      } else if (CT == NodeType::String) {
        Cmd.ExpectedMessage = stripQuotes(nodeText(Child));
      }
    }
    return Cmd;
  }

  ScriptCommand convertAssertModule(Node N, Cursor &C, CommandType Type) {
    ScriptCommand Cmd;
    Cmd.Type = Type;
    Cmd.Line = N.startRow() + 1;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      if (CT == NodeType::Sexpr) {
        auto CKW = firstKeyword(Child);
        if (CKW == "module"sv) {
          Cmd.ModType = detectModuleType(Child);
          Cmd.ModuleSource = nodeText(Child);
        } else {
          Cmd.ModType = ModuleType::Text;
          Cmd.ModuleSource = nodeText(Child);
        }
      } else if (CT == NodeType::String) {
        Cmd.ExpectedMessage = stripQuotes(nodeText(Child));
      }
    }
    return Cmd;
  }

  ScriptCommand convertAssertInvalid(Node N, Cursor &C) {
    return convertAssertModule(N, C, CommandType::AssertInvalid);
  }
  ScriptCommand convertAssertMalformed(Node N, Cursor &C) {
    return convertAssertModule(N, C, CommandType::AssertMalformed);
  }
  ScriptCommand convertAssertUnlinkable(Node N, Cursor &C) {
    return convertAssertModule(N, C, CommandType::AssertUnlinkable);
  }
  ScriptCommand convertAssertUninstantiable(Node N, Cursor &C) {
    return convertAssertModule(N, C, CommandType::AssertUninstantiable);
  }

  ScriptCommand convertAssertException(Node N, Cursor &C) {
    ScriptCommand Cmd;
    Cmd.Type = CommandType::AssertException;
    Cmd.Line = N.startRow() + 1;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (nodeType(Child) == NodeType::Sexpr) {
        auto CKW = firstKeyword(Child);
        if (CKW == "invoke"sv || CKW == "get"sv) {
          Cmd.Act = parseAction(Child);
        }
      }
    }
    return Cmd;
  }

  // --- Module type detection for embedded modules ---

  ModuleType detectModuleType(Node N) {
    for (Cursor C(N); C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      auto Text = nodeText(Child);
      if (CT == NodeType::Keyword) {
        if (Text == "binary"sv)
          return ModuleType::Binary;
        if (Text == "quote"sv)
          return ModuleType::Quote;
      }
    }
    return ModuleType::Text;
  }

  // --- Thread/Wait ---

  ScriptCommand convertThread(Node N, Cursor &C) {
    ScriptCommand Cmd;
    Cmd.Type = CommandType::Thread;
    Cmd.Line = N.startRow() + 1;

    std::map<std::string, std::string> RegisterMap;

    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);

      if (CT == NodeType::Id) {
        Cmd.ModuleName = parseIdentifier(nodeText(Child));
        continue;
      }
      if (CT != NodeType::Sexpr) {
        continue;
      }

      auto CKW = firstKeyword(Child);
      Cursor CC(Child);
      if (!CKW.empty()) {
        CC.next();
      }

      if (CKW == "shared"sv) {
        // (shared (module $name) ...) — scan Child's sub-sexprs for module
        for (Cursor SC(Child); SC.valid(); SC.next()) {
          Node Sub = SC.node();
          if (nodeType(Sub) != NodeType::Sexpr)
            continue;
          Cursor SubKW(Sub);
          Node SubKWNode = SubKW.node();
          if (nodeType(SubKWNode) != NodeType::Keyword)
            continue;
          if (nodeText(SubKWNode) != "module"sv)
            continue;
          for (Cursor MC(Sub); MC.valid(); MC.next()) {
            Node MNode = MC.node();
            if (nodeType(MNode) == NodeType::Id) {
              auto ModRef = parseIdentifier(nodeText(MNode));
              Cmd.SharedModules.emplace_back(std::string(ModRef),
                                             std::string(ModRef));
            }
          }
        }
      } else if (CKW == "register"sv) {
        std::string_view RegName;
        std::string_view ModRef;
        for (; CC.valid(); CC.next()) {
          Node Sub = CC.node();
          if (nodeType(Sub) == NodeType::String) {
            RegName = stripQuotes(nodeText(Sub));
          } else if (nodeType(Sub) == NodeType::Id) {
            ModRef = parseIdentifier(nodeText(Sub));
          }
        }
        if (!ModRef.empty() && !RegName.empty()) {
          RegisterMap.emplace(std::string(ModRef), std::string(RegName));
        }
        ScriptCommand Sub;
        Sub.Type = CommandType::Register;
        Sub.Line = Child.startRow() + 1;
        Sub.RegisterName = RegName;
        if (!ModRef.empty()) {
          Sub.ModuleName = ModRef;
        }
        Cmd.SubCommands.push_back(std::move(Sub));
      } else if (CKW == "module"sv) {
        ScriptCommand Sub;
        Sub.Type = CommandType::Module;
        Sub.Line = Child.startRow() + 1;
        Sub.ModType = ModuleType::Text;
        Sub.ModuleSource = nodeText(Child);
        for (; CC.valid(); CC.next()) {
          Node MC = CC.node();
          if (nodeType(MC) == NodeType::Id) {
            Sub.ModuleName = parseIdentifier(nodeText(MC));
          }
        }
        Cmd.SubCommands.push_back(std::move(Sub));
      } else if (CKW == "invoke"sv) {
        ScriptCommand Sub;
        Sub.Type = CommandType::Action;
        Sub.Line = Child.startRow() + 1;
        Action Act;
        Act.Type = ActionType::Invoke;
        for (; CC.valid(); CC.next()) {
          Node IC = CC.node();
          if (nodeType(IC) == NodeType::String) {
            if (auto S = parseString(nodeText(IC), false)) {
              Act.FieldName = std::move(*S);
            }
          } else if (nodeType(IC) == NodeType::Id) {
            Act.ModuleName = parseIdentifier(nodeText(IC));
          }
        }
        Sub.Act = std::move(Act);
        Cmd.SubCommands.push_back(std::move(Sub));
      }
    }

    for (auto &[OrigName, AliasName] : Cmd.SharedModules) {
      if (auto It = RegisterMap.find(OrigName); It != RegisterMap.end()) {
        AliasName = It->second;
      }
    }

    return Cmd;
  }

  ScriptCommand convertWait(Node N, Cursor &C) {
    ScriptCommand Cmd;
    Cmd.Type = CommandType::Wait;
    Cmd.Line = N.startRow() + 1;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (nodeType(Child) == NodeType::Id) {
        Cmd.ThreadName = parseIdentifier(nodeText(Child));
      }
    }
    return Cmd;
  }

  // --- Module source resolution ---

  void resolveModuleSource(ScriptCommand &Cmd) {
    if (Cmd.Type == CommandType::ModuleDefinition &&
        Cmd.ModType == ModuleType::Text) {
      auto Src = Cmd.ModuleSource;
      auto Pos = Src.find("definition");
      if (Pos != std::string_view::npos) {
        auto After = Pos + 10; // strlen("definition")
        while (After < Src.size() && Src[After] == ' ') {
          ++After;
        }
        OwnedStrings_->push_back(std::string(Src.substr(0, Pos)) +
                                 std::string(Src.substr(After)));
        Cmd.ModuleSource = OwnedStrings_->back();
      }
    }
    if (Cmd.ModType == ModuleType::Quote) {
      OwnedStrings_->push_back(extractQuotedStrings(Cmd.ModuleSource));
      Cmd.ModuleSource = OwnedStrings_->back();
      Cmd.ModType = ModuleType::Text;
    }
    if (Cmd.ModType == ModuleType::Binary) {
      OwnedStrings_->push_back(extractQuotedStrings(Cmd.ModuleSource));
      Cmd.ModuleSource = OwnedStrings_->back();
    }
  }

  // --- String decoding helpers ---

  static size_t findNextString(std::string_view Src, size_t &Pos) {
    while (Pos < Src.size()) {
      if (Src[Pos] == ' ' || Src[Pos] == '\t' || Src[Pos] == '\r' ||
          Src[Pos] == '\n') {
        ++Pos;
        continue;
      }
      if (Pos + 1 < Src.size() && Src[Pos] == ';' && Src[Pos + 1] == ';') {
        auto Eol = Src.find('\n', Pos);
        Pos = (Eol == std::string_view::npos) ? Src.size() : Eol + 1;
        continue;
      }
      if (Pos + 1 < Src.size() && Src[Pos] == '(' && Src[Pos + 1] == ';') {
        auto End = Src.find(";)", Pos + 2);
        Pos = (End == std::string_view::npos) ? Src.size() : End + 2;
        continue;
      }
      if (Src[Pos] == '"') {
        return Pos;
      }
      ++Pos;
    }
    return std::string_view::npos;
  }

  static std::string extractQuotedStrings(std::string_view RawSource) {
    std::string Result;
    size_t Pos = 0;
    while (Pos < RawSource.size()) {
      auto QStart = findNextString(RawSource, Pos);
      if (QStart == std::string_view::npos)
        break;
      auto QEnd = QStart + 1;
      while (QEnd < RawSource.size()) {
        if (RawSource[QEnd] == '\\') {
          QEnd += 2;
        } else if (RawSource[QEnd] == '"') {
          break;
        } else {
          ++QEnd;
        }
      }
      if (QEnd >= RawSource.size())
        break;
      auto StrLit = RawSource.substr(QStart, QEnd - QStart + 1);
      if (auto Str = parseString(StrLit, false)) {
        Result.append(*Str);
      }
      Pos = QEnd + 1;
    }
    return Result;
  }


  // State
  std::string_view Source_;
  std::deque<std::string> *OwnedStrings_ = nullptr;
};

// --- WastConverter::convert (out-of-line, after all helpers are declared) ---

Expect<WastScript> WastConverter::convert(const Tree &T, std::string Source) {
  WastScript Script;
  Script.Source = std::move(Source);
  Source_ = Script.Source;
  OwnedStrings_ = &Script.OwnedStrings;

  Node Root = T.rootNode();
  if (Root.hasError()) {
    spdlog::warn("WAST tree-sitter parse had errors"sv);
  }

  static const std::unordered_map<std::string_view, CmdKind, Hash::Hash>
      CmdKindMap = {
          {"module"sv, CmdKind::Module},
          {"register"sv, CmdKind::Register},
          {"invoke"sv, CmdKind::Invoke},
          {"get"sv, CmdKind::Get},
          {"assert_return"sv, CmdKind::AssertReturn},
          {"assert_trap"sv, CmdKind::AssertTrap},
          {"assert_exhaustion"sv, CmdKind::AssertExhaustion},
          {"assert_invalid"sv, CmdKind::AssertInvalid},
          {"assert_malformed"sv, CmdKind::AssertMalformed},
          {"assert_unlinkable"sv, CmdKind::AssertUnlinkable},
          {"assert_uninstantiable"sv, CmdKind::AssertUninstantiable},
          {"assert_exception"sv, CmdKind::AssertException},
          {"thread"sv, CmdKind::Thread},
          {"wait"sv, CmdKind::Wait},
      };

  for (Cursor RC(Root); RC.valid(); RC.next()) {
    Node Child = RC.node();
    if (nodeType(Child) != NodeType::Sexpr)
      continue;

    Cursor C(Child);
    Node KWNode = C.node();
    ScriptCommand Cmd;

    if (nodeType(KWNode) == NodeType::Keyword) {
      if (auto It = CmdKindMap.find(nodeText(KWNode)); It != CmdKindMap.end()) {
        C.next();
        switch (It->second) {
        case CmdKind::Module:
          Cmd = convertModule(Child, C);
          break;
        case CmdKind::Register:
          Cmd = convertRegister(Child, C);
          break;
        case CmdKind::Invoke:
          Cmd = convertActionCommand(Child, C, ActionType::Invoke);
          break;
        case CmdKind::Get:
          Cmd = convertActionCommand(Child, C, ActionType::Get);
          break;
        case CmdKind::AssertReturn:
          Cmd = convertAssertReturn(Child, C);
          break;
        case CmdKind::AssertTrap:
          Cmd = convertAssertTrap(Child, C);
          break;
        case CmdKind::AssertExhaustion:
          Cmd = convertAssertExhaustion(Child, C);
          break;
        case CmdKind::AssertInvalid:
          Cmd = convertAssertInvalid(Child, C);
          break;
        case CmdKind::AssertMalformed:
          Cmd = convertAssertMalformed(Child, C);
          break;
        case CmdKind::AssertUnlinkable:
          Cmd = convertAssertUnlinkable(Child, C);
          break;
        case CmdKind::AssertUninstantiable:
          Cmd = convertAssertUninstantiable(Child, C);
          break;
        case CmdKind::AssertException:
          Cmd = convertAssertException(Child, C);
          break;
        case CmdKind::Thread:
          Cmd = convertThread(Child, C);
          break;
        case CmdKind::Wait:
          Cmd = convertWait(Child, C);
          break;
        }
      } else {
        Cmd = convertBareModule(Child);
      }
    } else {
      Cmd = convertBareModule(Child);
    }

    resolveModuleSource(Cmd);
    Script.Commands.push_back(std::move(Cmd));
  }

  return Script;
}

} // anonymous namespace

// --- Public API ---

Expect<WastScript> parseWast(const std::filesystem::path &Path) {
  std::ifstream File(Path, std::ios::binary | std::ios::ate);
  if (!File.is_open()) {
    spdlog::error("Failed to open WAST file: {}"sv, Path.u8string());
    return Unexpect(ErrCode::Value::IllegalPath);
  }
  auto Size = File.tellg();
  File.seekg(0, std::ios::beg);

  std::string Source;
  Source.resize(static_cast<size_t>(Size));
  File.read(Source.data(), Size);
  File.close();

  Parser P(tree_sitter_wat);
  Tree T = P.parse(Source);
  if (T.rootNode().isNull()) {
    spdlog::error("Failed to parse WAST file: {}"sv, Path.u8string());
    return Unexpect(ErrCode::Value::IllegalPath);
  }

  WastConverter Conv;
  return Conv.convert(T, std::move(Source));
}

} // namespace Wast
} // namespace WasmEdge
