// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wast_parser.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "wat/wat_util.h"

#include "tree_sitter.h"

#include <cstddef>
#include <fstream>
#include <optional>
#include <utility>

namespace WasmEdge {
namespace Wast {

namespace {
using namespace WasmEdge::WAT; // Node, Cursor, Tree, Parser
using namespace std::literals;

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

// --- WastConverter ---
// This class walks a parse tree of the WAT grammar, which holds sexpr nodes
// and keyword nodes. It builds a WastScript. It gets the semantics when it
// matches the keyword strings, as the WAT Converter does.

class WastConverter {
public:
  Expect<WastScript> convert(const Tree &T, std::string Source);

private:
  // --- Helpers, with the same pattern as the WAT Converter ---

  std::string_view nodeText(Node N) const { return N.text(SourceView); }

  // Report a malformed expectation. The conversion then fails at the end,
  // so the test unit fails instead of a silent wrong expectation.
  void reportError(uint32_t Line, std::string_view Message) {
    spdlog::error("WAST line {}: {}"sv, Line, Message);
    HasError = true;
  }
  void reportError(Node N, std::string_view Message) {
    reportError(N.startRow() + 1, Message);
  }

  // Parse the literal Text with Parse. A literal that is not valid is a
  // malformed expectation, so report it on Where and give zero.
  template <typename T>
  T literal(Node Where, std::string_view Text,
            Expect<T> (*Parse)(std::string_view)) {
    if (auto V = Parse(Text)) {
      return *V;
    }
    reportError(Where, "literal is not valid or out of range");
    return T{};
  }

  // Take the one operand of a scalar constant from C. The operand is a number,
  // or a NaN pattern if AllowNaN is true. A missing operand, a second operand,
  // or an operand of another kind is a malformed expectation. The function
  // reports it, and then gives a null node.
  Node scalarOperand(Node Where, Cursor &C, bool AllowNaN) {
    Node Operand;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      const bool Accepted =
          isNumeric(Child) ||
          (AllowNaN && nanPattern(Child) != Result::NaNPattern::None);
      if (!Accepted || !Operand.isNull()) {
        reportError(Child, "unexpected operand of a constant");
        return {};
      }
      Operand = Child;
    }
    if (Operand.isNull()) {
      reportError(Where, "missing operand of a constant");
    }
    return Operand;
  }

  /// Return true if a node is a numeric token, which is u, s, or f.
  bool isNumeric(Node N) const {
    auto T = nodeType(N);
    return T == NodeType::U || T == NodeType::S || T == NodeType::F;
  }

  /// Give the NaN pattern of a node, for nan:canonical or nan:arithmetic.
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

  /// Return the keyword text of the first child of an sexpr node. If the first
  /// child is not a keyword, the function returns an empty string_view.
  std::string_view firstKeyword(Node N) const {
    Cursor C(N);
    Node K = C.node();
    return nodeType(K) == NodeType::Keyword ? nodeText(K) : std::string_view{};
  }

  // --- Heap type keyword -> TypeCode ---

  TypeCode heapTypeToCode(Node N) {
    static constexpr std::pair<std::string_view, TypeCode> HeapTypes[] = {
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
    if (const auto Code = lookupKeyword(HeapTypes, nodeText(N))) {
      return *Code;
    }
    reportError(N, "unknown heap type"sv);
    return TypeCode::FuncRef;
  }

  // --- Parse the const expressions, which are the invoke arguments ---

  template <typename T, TypeCode TC>
  void parseConstExprNumeric(Node N, Cursor &C, std::vector<ValVariant> &Args,
                             std::vector<ValType> &Types,
                             Expect<T> (*ParseFn)(std::string_view)) {
    const Node Operand = scalarOperand(N, C, false);
    Args.emplace_back(
        Operand.isNull() ? T{} : literal(Operand, nodeText(Operand), ParseFn));
    Types.emplace_back(TC);
  }

  void parseConstExprV128(Node N, Cursor &C, std::vector<ValVariant> &Args,
                          std::vector<ValType> &Types) {
    std::string_view Shape;
    std::vector<std::string_view> Lanes;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      if (CT == NodeType::Keyword && Shape.empty()) {
        Shape = nodeText(Child);
      } else if (isNumeric(Child) && !Shape.empty()) {
        Lanes.push_back(nodeText(Child));
      } else {
        reportError(Child, "unexpected operand of a constant");
      }
    }
    Args.emplace_back(packV128(N, Shape, Lanes, [](size_t) { return false; }));
    Types.emplace_back(TypeCode::V128);
  }

  void parseConstExprRefNull(Cursor &C, std::vector<ValVariant> &Args,
                             std::vector<ValType> &Types) {
    TypeCode Code = TypeCode::FuncRef;
    bool HasHeapType = false;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (nodeType(Child) == NodeType::Keyword && !HasHeapType) {
        HasHeapType = true;
        Code = heapTypeToCode(Child);
      } else {
        reportError(Child, "unexpected operand of a constant");
      }
    }
    Args.emplace_back(RefVariant(Code));
    Types.emplace_back(Code);
  }

  void parseConstExprRefOpaque(Node N, Cursor &C, std::vector<ValVariant> &Args,
                               std::vector<ValType> &Types, TypeCode Code) {
    const Node Operand = scalarOperand(N, C, false);
    const uint32_t Val = Operand.isNull() ? 0
                                          : literal(Operand, nodeText(Operand),
                                                    &parseInt<uint32_t>);
    Args.emplace_back(makeHostRef(Code, Val));
    Types.emplace_back(Code);
  }

  // A ref.func argument has no operand. It gives a null funcref, because a host
  // value cannot refer to a function of a module.
  void parseConstExprRefFunc(Cursor &C, std::vector<ValVariant> &Args,
                             std::vector<ValType> &Types) {
    if (C.valid()) {
      reportError(C.node(), "unexpected operand of a constant");
    }
    Args.emplace_back(RefVariant(TypeCode::FuncRef));
    Types.emplace_back(TypeCode::FuncRef);
  }

  void parseConstExpr(Node N, std::vector<ValVariant> &Args,
                      std::vector<ValType> &Types) {
    Cursor C(N);
    static constexpr std::pair<std::string_view, ConstKind> Kinds[] = {
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
    const auto Kind = lookupKeyword(Kinds, firstKeyword(N));
    if (!Kind) {
      reportError(N, "unknown constant expression");
      return;
    }
    C.next();
    switch (*Kind) {
    case ConstKind::I32:
      parseConstExprNumeric<uint32_t, TypeCode::I32>(N, C, Args, Types,
                                                     &parseInt<uint32_t>);
      break;
    case ConstKind::I64:
      parseConstExprNumeric<uint64_t, TypeCode::I64>(N, C, Args, Types,
                                                     &parseInt<uint64_t>);
      break;
    case ConstKind::F32:
      parseConstExprNumeric<uint32_t, TypeCode::F32>(N, C, Args, Types,
                                                     &parseFloatBits<uint32_t>);
      break;
    case ConstKind::F64:
      parseConstExprNumeric<uint64_t, TypeCode::F64>(N, C, Args, Types,
                                                     &parseFloatBits<uint64_t>);
      break;
    case ConstKind::V128:
      parseConstExprV128(N, C, Args, Types);
      break;
    case ConstKind::RefNull:
      parseConstExprRefNull(C, Args, Types);
      break;
    case ConstKind::RefExtern:
      parseConstExprRefOpaque(N, C, Args, Types, TypeCode::ExternRef);
      break;
    case ConstKind::RefHost:
      parseConstExprRefOpaque(N, C, Args, Types, TypeCode::AnyRef);
      break;
    case ConstKind::RefFunc:
      parseConstExprRefFunc(C, Args, Types);
      break;
    }
  }

  // --- V128 lane parsing helper ---

  template <typename UT, typename SkipFn>
  void packLanes(Node Where, const std::vector<std::string_view> &Lanes,
                 uint128_t &V128, Expect<UT> (*Parse)(std::string_view),
                 SkipFn Skip) {
    for (size_t K = 0; K < Lanes.size(); ++K) {
      if (!Skip(K)) {
        setV128Lane(V128, K, literal(Where, Lanes[K], Parse));
      }
    }
  }

  // Pack the lanes of a v128 of the shape Shape. If Skip(K) is true, lane K
  // stays zero. A wrong shape, a wrong lane count, or a lane that is not
  // valid is a malformed expectation, and the function reports it on Where.
  template <typename SkipFn>
  uint128_t packV128(Node Where, std::string_view Shape,
                     const std::vector<std::string_view> &Lanes, SkipFn Skip) {
    uint128_t V128{};
    if (Shape == "i8x16"sv && Lanes.size() == 16) {
      packLanes(Where, Lanes, V128, &parseInt<uint8_t>, Skip);
    } else if (Shape == "i16x8"sv && Lanes.size() == 8) {
      packLanes(Where, Lanes, V128, &parseInt<uint16_t>, Skip);
    } else if (Shape == "i32x4"sv && Lanes.size() == 4) {
      packLanes(Where, Lanes, V128, &parseInt<uint32_t>, Skip);
    } else if (Shape == "i64x2"sv && Lanes.size() == 2) {
      packLanes(Where, Lanes, V128, &parseInt<uint64_t>, Skip);
    } else if (Shape == "f32x4"sv && Lanes.size() == 4) {
      packLanes(Where, Lanes, V128, &parseFloatBits<uint32_t>, Skip);
    } else if (Shape == "f64x2"sv && Lanes.size() == 2) {
      packLanes(Where, Lanes, V128, &parseFloatBits<uint64_t>, Skip);
    } else {
      reportError(Where, "v128 shape or lane count is not valid");
    }
    return V128;
  }

  // --- Parse the actions, which are invoke and get ---

  // action ::= (invoke id? name const*) | (get id? name)
  // The fields come in this order, and each one at most once, so that a
  // damaged action fails instead of a call of another export.
  Action parseAction(Node N) {
    Action Act;
    Act.Type = ActionType::Get;
    Cursor C(N);
    if (nodeType(C.node()) == NodeType::Keyword) {
      if (nodeText(C.node()) == "invoke"sv) {
        Act.Type = ActionType::Invoke;
      }
      C.next();
    }
    if (nodeType(C.node()) == NodeType::Id) {
      Act.ModuleName = identifier(C.node());
      C.next();
    }
    if (nodeType(C.node()) == NodeType::String) {
      Act.FieldName = std::string(decodeQuoted(C.node()));
      C.next();
    } else {
      reportError(N, "the action has no field name");
    }
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (Act.Type == ActionType::Invoke &&
          nodeType(Child) == NodeType::Sexpr) {
        parseConstExpr(Child, Act.Args, Act.ArgTypes);
      } else {
        reportError(Child, "unexpected operand of the action");
      }
    }
    return Act;
  }

  // --- Parse the results ---

  // Parse a scalar result of the type TC. A float result can also be a NaN
  // pattern.
  template <typename T, TypeCode TC>
  Result parseResultScalar(Node N, Cursor &C,
                           Expect<T> (*ParseFn)(std::string_view)) {
    constexpr bool IsFloat = TC == TypeCode::F32 || TC == TypeCode::F64;
    Result R;
    R.Type = ValType(TC);
    R.Value = T(0);
    const Node Operand = scalarOperand(N, C, IsFloat);
    if (Operand.isNull()) {
      return R;
    }
    if (auto NaN = nanPattern(Operand); NaN != Result::NaNPattern::None) {
      R.NaN = NaN;
    } else {
      R.Value = literal(Operand, nodeText(Operand), ParseFn);
    }
    return R;
  }

  Result parseResultV128(Cursor &C) {
    Result R;
    R.Type = ValType(TypeCode::V128);
    std::string_view Shape;
    std::vector<std::string_view> Lanes;
    const Node Where = C.node();

    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      if (Shape.empty()) {
        if (CT == NodeType::Keyword) {
          Shape = nodeText(Child);
        } else {
          reportError(Child, "unexpected operand of a constant");
        }
      } else if (auto NaN = nanPattern(Child);
                 NaN != Result::NaNPattern::None) {
        Lanes.emplace_back();
        R.V128LaneNaN.push_back(NaN);
      } else if (isNumeric(Child)) {
        Lanes.push_back(nodeText(Child));
        R.V128LaneNaN.push_back(Result::NaNPattern::None);
      } else {
        reportError(Child, "unexpected operand of a constant");
      }
    }

    R.V128Shape = std::string(Shape);
    R.Value = packV128(Where, Shape, Lanes, [&R](size_t K) {
      return R.V128LaneNaN[K] != Result::NaNPattern::None;
    });
    return R;
  }

  Result parseResultRefNull(Cursor &C) {
    Result R;
    TypeCode Code = TypeCode::FuncRef;
    bool HasHeapType = false;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      if (HasHeapType || (CT != NodeType::Keyword && !isNumeric(Child))) {
        reportError(Child, "unexpected operand of a constant");
      } else if (CT == NodeType::Keyword) {
        Code = heapTypeToCode(Child);
      }
      HasHeapType = true;
    }
    if (!HasHeapType) {
      Code = TypeCode::AnyRef;
      R.AnyNullRef = true;
    }
    R.Type = ValType(Code);
    R.Value = RefVariant(Code);
    return R;
  }

  // A host reference result without an operand matches any reference of the
  // type.
  Result parseResultHostRef(Cursor &C, TypeCode Code) {
    if (!C.valid()) {
      return makeOpaqueRefResult(Code);
    }
    const Node Operand = scalarOperand(C.node(), C, false);
    const uint32_t Val = Operand.isNull() ? 0
                                          : literal(Operand, nodeText(Operand),
                                                    &parseInt<uint32_t>);
    Result R;
    R.Type = ValType(Code);
    R.Value = makeHostRef(Code, Val);
    return R;
  }

  Result makeOpaqueRefResult(TypeCode Code) {
    Result R;
    R.Type = ValType(Code);
    R.Value = RefVariant(Code);
    R.OpaqueRef = true;
    return R;
  }

  Result parseResultConst(Node N) {
    Cursor C(N);
    static constexpr std::pair<std::string_view, ResultKind> Kinds[] = {
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
    const auto Kind = lookupKeyword(Kinds, firstKeyword(N));
    if (!Kind) {
      reportError(N, "unknown result expression");
      return Result{};
    }
    C.next();
    switch (*Kind) {
    case ResultKind::I32:
      return parseResultScalar<uint32_t, TypeCode::I32>(N, C,
                                                        &parseInt<uint32_t>);
    case ResultKind::I64:
      return parseResultScalar<uint64_t, TypeCode::I64>(N, C,
                                                        &parseInt<uint64_t>);
    case ResultKind::F32:
      return parseResultScalar<uint32_t, TypeCode::F32>(
          N, C, &parseFloatBits<uint32_t>);
    case ResultKind::F64:
      return parseResultScalar<uint64_t, TypeCode::F64>(
          N, C, &parseFloatBits<uint64_t>);
    case ResultKind::V128:
      return parseResultV128(C);
    case ResultKind::RefNull:
      return parseResultRefNull(C);
    case ResultKind::RefExtern:
      return parseResultHostRef(C, TypeCode::ExternRef);
    case ResultKind::RefHost:
      return parseResultHostRef(C, TypeCode::AnyRef);
    case ResultKind::RefFunc:
      return makeOpaqueRefResult(TypeCode::FuncRef);
    case ResultKind::RefAny:
      return makeOpaqueRefResult(TypeCode::AnyRef);
    case ResultKind::RefEq:
      return makeOpaqueRefResult(TypeCode::EqRef);
    case ResultKind::RefI31:
      return makeOpaqueRefResult(TypeCode::I31Ref);
    case ResultKind::RefStruct:
      return makeOpaqueRefResult(TypeCode::StructRef);
    case ResultKind::RefArray:
      return makeOpaqueRefResult(TypeCode::ArrayRef);
    case ResultKind::RefExn:
      return makeOpaqueRefResult(TypeCode::ExnRef);
    }
    return Result{};
  }

  // --- Command converters ---

  static ScriptCommand newCommand(Node N, CommandType Type) {
    ScriptCommand Cmd;
    Cmd.Type = Type;
    Cmd.Line = N.startRow() + 1;
    return Cmd;
  }

  ScriptCommand convertModule(Node N, Cursor &C) {
    ScriptCommand Cmd = newCommand(N, CommandType::Module);
    Cmd.ModType = detectModuleType(N, Cmd.ModuleSource);

    std::string_view DefinitionKeyword;
    std::vector<std::string_view> Identifiers;

    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      auto Text = nodeText(Child);

      if (CT == NodeType::Id) {
        Identifiers.push_back(identifier(Child));
      } else if (CT == NodeType::Keyword) {
        if (Text == "definition"sv) {
          Cmd.Type = CommandType::ModuleDefinition;
          DefinitionKeyword = Text;
        } else if (Text == "instance"sv) {
          Cmd.Type = CommandType::ModuleInstance;
        }
      }
    }

    if (Cmd.ModType == ModuleType::Text && !DefinitionKeyword.empty()) {
      // The module text of a definition is a plain module. Remove the
      // "definition" keyword and the spaces after it.
      const auto Src = Cmd.ModuleSource;
      const auto Pos =
          static_cast<size_t>(DefinitionKeyword.data() - Src.data());
      auto After = Pos + DefinitionKeyword.size();
      while (After < Src.size() && Src[After] == ' ') {
        ++After;
      }
      OwnedStrings->push_back(std::string(Src.substr(0, Pos)) +
                              std::string(Src.substr(After)));
      Cmd.ModuleSource = OwnedStrings->back();
    }

    if (Cmd.Type == CommandType::ModuleInstance) {
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

  ScriptCommand convertRegister(Node N, Cursor &C) {
    ScriptCommand Cmd = newCommand(N, CommandType::Register);
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      if (CT == NodeType::String) {
        Cmd.RegisterName = decodeQuoted(Child);
      } else if (CT == NodeType::Id) {
        Cmd.ModuleName = identifier(Child);
      }
    }
    return Cmd;
  }

  ScriptCommand convertActionCommand(Node N) {
    ScriptCommand Cmd = newCommand(N, CommandType::Action);
    Cmd.Act = parseAction(N);
    return Cmd;
  }

  // The form is (assert_return action result*). The action must come first,
  // so that a damaged command fails instead of a skipped assertion.
  ScriptCommand convertAssertReturn(Node N, Cursor &C) {
    ScriptCommand Cmd = newCommand(N, CommandType::AssertReturn);
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (nodeType(Child) != NodeType::Sexpr) {
        reportError(Child, "unexpected token in assert_return");
        continue;
      }

      auto CKW = firstKeyword(Child);

      if (CKW == "invoke"sv || CKW == "get"sv) {
        if (Cmd.Act) {
          reportError(Child, "assert_return has more than one action");
        } else if (!Cmd.Expected.empty()) {
          reportError(Child, "the action of assert_return must come first");
        }
        Cmd.Act = parseAction(Child);
      } else if (!Cmd.Act) {
        reportError(Child, "the action of assert_return must come first");
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
    if (!Cmd.Act) {
      reportError(N, "assert_return has no action");
    }
    return Cmd;
  }

  // Convert an assertion other than assert_return. An invoke or a get is the
  // action, any other sexpr is the module, and a string is the expected
  // message. The executor reads only the fields that the command type uses.
  // The assertion holds exactly one action or module, then one message.
  // assert_exception has no message.
  ScriptCommand convertAssertion(Node N, Cursor &C, CommandType Type) {
    ScriptCommand Cmd = newCommand(N, Type);
    bool HasOperand = false;
    bool HasMessage = false;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      if (CT == NodeType::Sexpr) {
        if (HasOperand || HasMessage) {
          reportError(Child, "unexpected operand in the assertion");
        }
        HasOperand = true;
        auto CKW = firstKeyword(Child);
        if (CKW == "invoke"sv || CKW == "get"sv) {
          Cmd.Act = parseAction(Child);
        } else if (CKW == "module"sv) {
          Cmd.ModType = detectModuleType(Child, Cmd.ModuleSource);
        } else {
          Cmd.ModType = ModuleType::Text;
          Cmd.ModuleSource = nodeText(Child);
        }
      } else if (CT == NodeType::String) {
        if (HasMessage || !HasOperand) {
          reportError(Child, "unexpected message in the assertion");
        }
        HasMessage = true;
        Cmd.ExpectedMessage = decodeQuoted(Child);
      } else {
        reportError(Child, "unexpected token in the assertion");
      }
    }
    const bool NeedsAction = Type == CommandType::AssertExhaustion ||
                             Type == CommandType::AssertException;
    const bool AllowsAction = NeedsAction || Type == CommandType::AssertTrap;
    if (!HasOperand || (NeedsAction && !Cmd.Act) ||
        (!AllowsAction && Cmd.Act)) {
      reportError(N, "the assertion has no valid action or module");
    }
    if (HasMessage == (Type == CommandType::AssertException)) {
      reportError(N, Type == CommandType::AssertException
                         ? "assert_exception has a message"sv
                         : "the assertion has no message"sv);
    }
    return Cmd;
  }

  // --- Find the module type of an embedded module ---

  ModuleType detectModuleType(Node N, std::string_view &Source) {
    Source = nodeText(N);
    for (Cursor C(N); C.valid(); C.next()) {
      Node Child = C.node();
      if (nodeType(Child) != NodeType::Keyword) {
        continue;
      }
      auto Text = nodeText(Child);
      if (Text == "binary"sv || Text == "quote"sv) {
        Source = textAfter(Child, N);
        return Text == "binary"sv ? ModuleType::Binary : ModuleType::Quote;
      }
    }
    return ModuleType::Text;
  }

  // Give the text of Parent after its child Child. For a binary or a quote
  // module, this text holds only the strings of the module, and not a quoted
  // module id.
  std::string_view textAfter(Node Child, Node Parent) const {
    return SourceView.substr(Child.endByte(),
                             Parent.endByte() - Child.endByte());
  }

  // --- Thread/Wait ---

  // Give the command kind of the sexpr N, or nothing if N is not a command.
  std::optional<CommandType> commandKind(Node N) const {
    static constexpr std::pair<std::string_view, CommandType> Kinds[] = {
        {"module"sv, CommandType::Module},
        {"register"sv, CommandType::Register},
        {"invoke"sv, CommandType::Action},
        {"get"sv, CommandType::Action},
        {"assert_return"sv, CommandType::AssertReturn},
        {"assert_trap"sv, CommandType::AssertTrap},
        {"assert_exhaustion"sv, CommandType::AssertExhaustion},
        {"assert_invalid"sv, CommandType::AssertInvalid},
        {"assert_malformed"sv, CommandType::AssertMalformed},
        {"assert_unlinkable"sv, CommandType::AssertUnlinkable},
        {"assert_uninstantiable"sv, CommandType::AssertUninstantiable},
        {"assert_exception"sv, CommandType::AssertException},
        {"thread"sv, CommandType::Thread},
        {"wait"sv, CommandType::Wait},
    };
    return lookupKeyword(Kinds, firstKeyword(N));
  }

  // Convert one command sexpr. The top level and a thread body both use
  // this function. A sexpr that is not a command is an error. The script then
  // fails, so the empty command that this function gives never runs.
  ScriptCommand convertCommand(Node Child) {
    const auto Kind = commandKind(Child);
    if (!Kind) {
      reportError(Child, "unknown command");
      return ScriptCommand{};
    }
    Cursor C(Child);
    C.next();

    ScriptCommand Cmd;
    switch (*Kind) {
    case CommandType::Module:
    case CommandType::ModuleDefinition:
    case CommandType::ModuleInstance:
      Cmd = convertModule(Child, C);
      break;
    case CommandType::Register:
      Cmd = convertRegister(Child, C);
      break;
    case CommandType::Action:
      Cmd = convertActionCommand(Child);
      break;
    case CommandType::AssertReturn:
      Cmd = convertAssertReturn(Child, C);
      break;
    case CommandType::AssertTrap:
    case CommandType::AssertExhaustion:
    case CommandType::AssertInvalid:
    case CommandType::AssertMalformed:
    case CommandType::AssertUnlinkable:
    case CommandType::AssertUninstantiable:
    case CommandType::AssertException:
      Cmd = convertAssertion(Child, C, *Kind);
      break;
    case CommandType::Thread:
      Cmd = convertThread(Child, C);
      break;
    case CommandType::Wait:
      Cmd = convertWait(Child, C);
      break;
    }

    resolveModuleSource(Cmd);
    return Cmd;
  }

  ScriptCommand convertThread(Node N, Cursor &C) {
    ScriptCommand Cmd = newCommand(N, CommandType::Thread);

    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);

      if (CT == NodeType::Id) {
        Cmd.ThreadName = identifier(Child);
        continue;
      }
      if (CT != NodeType::Sexpr) {
        continue;
      }

      if (firstKeyword(Child) == "shared"sv) {
        // The form is (shared (module $name) ...). Scan the sub-sexprs of
        // Child for a module.
        for (Cursor SC(Child); SC.valid(); SC.next()) {
          Node Sub = SC.node();
          if (nodeType(Sub) != NodeType::Sexpr ||
              firstKeyword(Sub) != "module"sv) {
            continue;
          }
          for (Cursor MC(Sub); MC.valid(); MC.next()) {
            Node MNode = MC.node();
            if (nodeType(MNode) == NodeType::Id) {
              // The executor finds the alias name of the module in the
              // register commands of the thread.
              Cmd.SharedModules.emplace_back(identifier(MNode));
            }
          }
        }
        continue;
      }

      // Every other command of the thread body goes through the same
      // conversion as a top-level command. As a result, an assertion inside
      // a thread runs, and an invoke gets its arguments.
      Cmd.SubCommands.push_back(convertCommand(Child));
    }
    return Cmd;
  }

  ScriptCommand convertWait(Node N, Cursor &C) {
    ScriptCommand Cmd = newCommand(N, CommandType::Wait);
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (nodeType(Child) == NodeType::Id) {
        Cmd.ThreadName = identifier(Child);
      }
    }
    return Cmd;
  }

  // --- Resolve the module source ---

  void resolveModuleSource(ScriptCommand &Cmd) {
    if (Cmd.ModType != ModuleType::Quote && Cmd.ModType != ModuleType::Binary) {
      return;
    }
    auto Bytes = extractQuotedStrings(Cmd.ModuleSource);
    if (!Bytes) {
      reportError(Cmd.Line, "the module has a string that is not valid");
      return;
    }
    OwnedStrings->push_back(std::move(*Bytes));
    Cmd.ModuleSource = OwnedStrings->back();
    if (Cmd.ModType == ModuleType::Quote) {
      Cmd.ModType = ModuleType::Text;
    }
  }

  // --- Helpers that decode the strings ---

  static size_t findNextString(std::string_view Src, size_t &Pos) {
    while (Pos < Src.size()) {
      if (isWatSpace(static_cast<unsigned char>(Src[Pos]))) {
        ++Pos;
        continue;
      }
      if (Pos + 1 < Src.size() && Src[Pos] == ';' && Src[Pos + 1] == ';') {
        Pos = lineCommentEnd(Src, Pos + 2);
        continue;
      }
      if (Pos + 1 < Src.size() && Src[Pos] == '(' && Src[Pos + 1] == ';') {
        // A WAT block comment can nest, for example (; a (; b ;) c ;). Track
        // the depth. A quote in the tail of the comment, after an inner ;),
        // is then not a real string token. Such a quote must not get into the
        // bytes of the module that this code rebuilds.
        Pos = blockCommentEnd(Src, Pos + 2);
        continue;
      }
      if (Src[Pos] == '"') {
        return Pos;
      }
      ++Pos;
    }
    return std::string_view::npos;
  }

  static std::optional<std::string>
  extractQuotedStrings(std::string_view RawSource) {
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
      if (QEnd >= RawSource.size()) {
        // The string is not closed.
        return std::nullopt;
      }
      auto StrLit = RawSource.substr(QStart, QEnd - QStart + 1);
      auto Str = parseString(StrLit, /*ValidateUTF8=*/false);
      if (!Str) {
        return std::nullopt;
      }
      Result.append(*Str);
      Pos = QEnd + 1;
    }
    return Result;
  }

  // Decode a quoted token, which is a register name or a trap message. This
  // function resolves the escapes, so that the token is equal to the decoded
  // string of the runtime. The raw lexeme keeps its backslash escapes, for
  // example \"/\5c. OwnedStrings owns the result, so that the view stays
  // valid. A token that is not a valid string is an error.
  std::string_view decodeQuoted(Node N) {
    return own(N, parseString(nodeText(N), /*ValidateUTF8=*/false),
               "the string is not valid"sv);
  }

  // Decode the identifier N, as the WAT converter does ($abc -> abc,
  // $"a\62" -> ab). OwnedStrings owns the result, so that the view stays
  // valid. An identifier that is not valid is an error.
  std::string_view identifier(Node N) {
    return own(N, decodeIdentifier(nodeText(N)),
               "the identifier is not valid"sv);
  }

  // Keep the decoded token S of the node N in OwnedStrings, and give a view
  // of it. If the decoding failed, report Message on N and give an empty
  // view.
  std::string_view own(Node N, Expect<std::string> S,
                       std::string_view Message) {
    if (!S) {
      reportError(N, Message);
      return {};
    }
    OwnedStrings->push_back(std::move(*S));
    return OwnedStrings->back();
  }

  // State
  std::string_view SourceView;
  std::deque<std::string> *OwnedStrings = nullptr;
  bool HasError = false;
};

// --- WastConverter::convert, which comes after the helpers ---

Expect<WastScript> WastConverter::convert(const Tree &T, std::string Source) {
  WastScript Script;
  Script.Source = std::make_unique<std::string>(std::move(Source));
  SourceView = *Script.Source;
  OwnedStrings = &Script.OwnedStrings;

  // A syntax error can hide a command, so the script is not valid. A
  // malformed module of an assertion is in a string, and it does not make an
  // error here.
  Node Root = T.rootNode();
  if (Root.hasError()) {
    reportError(Root, "the script has a syntax error");
  }

  Cursor RC(Root);
  if (RC.valid() && nodeType(RC.node()) == NodeType::Sexpr &&
      !commandKind(RC.node())) {
    // A script that starts with a module field is one module in the bare
    // form. All the top-level sexprs are the fields of this module.
    Node First = RC.node();
    Node Last = First;
    for (; RC.valid(); RC.next()) {
      Last = RC.node();
    }
    ScriptCommand Cmd = newCommand(First, CommandType::Module);
    Cmd.ModType = ModuleType::Text;
    Cmd.ModuleSource = SourceView.substr(First.startByte(),
                                         Last.endByte() - First.startByte());
    Script.Commands.push_back(std::move(Cmd));
  } else {
    for (; RC.valid(); RC.next()) {
      Node Child = RC.node();
      if (nodeType(Child) != NodeType::Sexpr) {
        reportError(Child, "unexpected token at the top level");
        continue;
      }
      Script.Commands.push_back(convertCommand(Child));
    }
  }
  if (HasError) {
    return Unexpect(ErrCode::Value::IllegalGrammar);
  }

  return Script;
}

} // anonymous namespace

// --- Public API ---

Expect<WastScript> parseWast(const std::filesystem::path &Path) {
  std::ifstream File(Path, std::ios::binary | std::ios::ate);
  if (!File.is_open()) {
    spdlog::error("Failed to open WAST file: {}"sv, u8string(Path));
    return Unexpect(ErrCode::Value::IllegalPath);
  }
  const std::streamoff Size = File.tellg();
  if (Size < 0) {
    spdlog::error("Failed to determine WAST file size: {}"sv, u8string(Path));
    return Unexpect(ErrCode::Value::IllegalPath);
  }
  File.seekg(0, std::ios::beg);

  std::string Source;
  Source.resize(static_cast<size_t>(Size));
  File.read(Source.data(), Size);
  if (File.gcount() != Size) {
    spdlog::error("Failed to read WAST file: {} (read {} of {} bytes)"sv,
                  u8string(Path), static_cast<int64_t>(File.gcount()),
                  static_cast<int64_t>(Size));
    return Unexpect(ErrCode::Value::IllegalPath);
  }
  File.close();

  Parser P(tree_sitter_wat);
  Tree T = P.parse(Source);
  if (T.rootNode().isNull()) {
    spdlog::error("Failed to parse WAST file: {}"sv, u8string(Path));
    return Unexpect(ErrCode::Value::IllegalGrammar);
  }

  WastConverter Conv;
  return Conv.convert(T, std::move(Source));
}

} // namespace Wast
} // namespace WasmEdge
