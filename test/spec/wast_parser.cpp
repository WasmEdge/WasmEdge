// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wast_parser.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "wat/lexical.h"
#include "wat/wat_util.h"

#include "wat/tree_sitter.h"

#include <cstddef>
#include <fstream>
#include <optional>
#include <utility>
#include <vector>

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
  Ref,
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

  // A ref.null argument needs an abstract heap type. The harness has no
  // module types, so an indexed heap type is an error.
  void parseConstExprRefNull(Node N, Cursor &C, std::vector<ValVariant> &Args,
                             std::vector<ValType> &Types) {
    TypeCode Code = TypeCode::FuncRef;
    bool HasHeapType = false;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (nodeType(Child) == NodeType::Keyword && !HasHeapType) {
        HasHeapType = true;
        Code = heapTypeToCode(Child);
      } else if (!HasHeapType) {
        HasHeapType = true;
        reportError(Child, "unsupported heap type of a constant");
      } else {
        reportError(Child, "unexpected operand of a constant");
      }
    }
    if (!HasHeapType) {
      reportError(N, "ref.null needs a heap type");
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
      parseConstExprRefNull(N, C, Args, Types);
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

  // Pack the lanes of a v128 of the shape Shape. If Skip(K) is true, lane K
  // stays zero. A wrong shape, a wrong lane count, or a lane that is not
  // valid is a malformed expectation, and the function reports it on Where.
  // The shapes and the lane encoding come from the WAT parser, and the NaN
  // patterns that Skip selects stay in this harness.
  template <typename SkipFn>
  uint128_t packV128(Node Where, std::string_view Shape,
                     const std::vector<std::string_view> &Lanes, SkipFn Skip) {
    uint128_t V128{};
    const auto S = WAT::simdShapeOf(Shape);
    if (!S || Lanes.size() != WAT::simdShapeLanes(*S)) {
      reportError(Where, "v128 shape or lane count is not valid");
      return V128;
    }
    for (size_t K = 0; K < Lanes.size(); ++K) {
      if (!Skip(K) && !WAT::setSimdLane(V128, *S, K, Lanes[K])) {
        reportError(Where, "literal is not valid or out of range");
      }
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

  // A bare ref.null result matches any null reference. The harness has no
  // module types, so an indexed heap type is an error.
  Result parseResultRefNull(Cursor &C) {
    Result R;
    TypeCode Code = TypeCode::FuncRef;
    bool HasHeapType = false;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (HasHeapType) {
        reportError(Child, "unexpected operand of a constant");
      } else if (nodeType(Child) == NodeType::Keyword) {
        Code = heapTypeToCode(Child);
      } else {
        reportError(Child, "unsupported heap type of a constant");
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

  // An opaque reference result, such as ref.func, has no operand.
  Result parseResultOpaqueRef(Cursor &C, TypeCode Code) {
    for (; C.valid(); C.next()) {
      reportError(C.node(), "unexpected operand of a constant");
    }
    return makeOpaqueRefResult(Code);
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
        {"ref"sv, ResultKind::Ref},
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
      return parseResultOpaqueRef(C, TypeCode::FuncRef);
    case ResultKind::Ref: {
      Result R = parseResultOpaqueRef(C, TypeCode::AnyRef);
      R.AnyNonNullRef = true;
      return R;
    }
    case ResultKind::RefEq:
      return parseResultOpaqueRef(C, TypeCode::EqRef);
    case ResultKind::RefI31:
      return parseResultOpaqueRef(C, TypeCode::I31Ref);
    case ResultKind::RefStruct:
      return parseResultOpaqueRef(C, TypeCode::StructRef);
    case ResultKind::RefArray:
      return parseResultOpaqueRef(C, TypeCode::ArrayRef);
    case ResultKind::RefExn:
      return parseResultOpaqueRef(C, TypeCode::ExnRef);
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

  ScriptCommand convertModule(Node N, Cursor &) {
    ScriptCommand Cmd = newCommand(N, CommandType::Module);
    parseModuleHeader(N, Cmd);
    return Cmd;
  }

  ScriptCommand convertRegister(Node N, Cursor &C) {
    ScriptCommand Cmd = newCommand(N, CommandType::Register);
    bool HasName = false;
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      auto CT = nodeType(Child);
      if (CT == NodeType::String && !HasName) {
        Cmd.RegisterName = decodeQuoted(Child);
        HasName = true;
      } else if (CT == NodeType::Id && HasName && !Cmd.ModuleName) {
        Cmd.ModuleName = identifier(Child);
      } else {
        reportError(Child, "unexpected token in register");
      }
    }
    if (!HasName) {
      reportError(N, "register has no name");
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
        Cursor EC(Child);
        for (EC.next(); EC.valid(); EC.next()) {
          Node EChild = EC.node();
          if (nodeType(EChild) == NodeType::Sexpr) {
            ROE.Alternatives.push_back(parseResultConst(EChild));
          } else {
            reportError(EChild, "unexpected token in either");
          }
        }
        if (ROE.Alternatives.empty()) {
          reportError(Child, "either has no result");
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
  // action, a module sexpr is the module, and a string is the expected
  // message. Any other sexpr is an error of the script, and it never reaches
  // the loader. The executor reads only the fields that the command type uses.
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
          parseModuleHeader(Child, Cmd);
        } else {
          reportError(Child, "unexpected operand in the assertion");
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

  // --- Read the header of a module ---

  // Read the module sexpr N into Cmd. The words after the module keyword have
  // fixed positions: an optional "definition" or "instance" keyword, the ids,
  // then an optional "binary" or "quote" keyword. A binary or a quoted module
  // holds only strings after its keyword, and ModuleSource gets their decoded
  // bytes. A text module keeps its text in ModuleSource, and the WAT
  // converter reports the errors in its fields. A module command and the
  // module of an assertion both use this function, so a damaged header always
  // fails the script, and never reaches the loader.
  void parseModuleHeader(Node N, ScriptCommand &Cmd) {
    const bool IsCommand = Cmd.Type == CommandType::Module;
    Cmd.ModType = ModuleType::Text;
    Cmd.ModuleSource = nodeText(N);
    Cursor C(N);
    C.next();
    auto AtKeyword = [this, &C](std::string_view Keyword) {
      return C.valid() && nodeType(C.node()) == NodeType::Keyword &&
             nodeText(C.node()) == Keyword;
    };

    // An assertion that instantiates a module can take a module instance, as
    // in the script grammar of the spec. The other assertions take a module.
    const bool InstantiatesModule =
        Cmd.Type == CommandType::AssertUnlinkable ||
        Cmd.Type == CommandType::AssertUninstantiable ||
        Cmd.Type == CommandType::AssertTrap;
    if (AtKeyword("instance"sv)) {
      if (IsCommand) {
        Cmd.Type = CommandType::ModuleInstance;
      } else if (InstantiatesModule) {
        Cmd.InstanceOperand = true;
      } else {
        reportError(C.node(), "the assertion has no module instance");
      }
      Cmd.ModuleSource = {};
      std::vector<std::string_view> Ids;
      for (C.next(); C.valid(); C.next()) {
        if (nodeType(C.node()) == NodeType::Id && Ids.size() < 2) {
          Ids.push_back(identifier(C.node()));
        } else {
          reportError(C.node(), "unexpected token in the module instance");
        }
      }
      // The script grammar reads one id as the definition, and two ids as
      // the instance and the definition. An instance with no definition id
      // uses the latest definition. An assertion keeps no instance, so it
      // does not use the instance id, as in the reference interpreter.
      if (Ids.size() == 2) {
        if (IsCommand) {
          Cmd.ModuleName = Ids[0];
        }
        Cmd.DefinitionName = Ids[1];
      } else if (Ids.size() == 1) {
        Cmd.DefinitionName = Ids[0];
      }
      return;
    }

    std::string_view DefinitionKeyword;
    if (AtKeyword("definition"sv)) {
      DefinitionKeyword = nodeText(C.node());
      if (IsCommand) {
        Cmd.Type = CommandType::ModuleDefinition;
      } else if (InstantiatesModule) {
        // An assertion that instantiates a module cannot take a definition,
        // because a definition makes no instance.
        reportError(C.node(), "misplaced module definition");
      }
      C.next();
    }
    if (C.valid() && nodeType(C.node()) == NodeType::Id) {
      if (IsCommand) {
        Cmd.ModuleName = identifier(C.node());
      }
      C.next();
    }

    if (AtKeyword("binary"sv) || AtKeyword("quote"sv)) {
      Cmd.ModType = nodeText(C.node()) == "binary"sv ? ModuleType::Binary
                                                     : ModuleType::Quote;
      std::string Bytes;
      for (C.next(); C.valid(); C.next()) {
        Node Child = C.node();
        if (nodeType(Child) != NodeType::String) {
          reportError(Child, "unexpected token in the module");
          continue;
        }
        auto Str = parseString(nodeText(Child), /*ValidateUTF8=*/false);
        if (!Str) {
          reportError(Child, "the module has a string that is not valid");
          continue;
        }
        Bytes += *Str;
      }
      Script.OwnedStrings.push_back(std::move(Bytes));
      Cmd.ModuleSource = Script.OwnedStrings.back();
      return;
    }

    if (!DefinitionKeyword.empty()) {
      // The module text of a definition is a plain module. Remove the
      // "definition" keyword and the spaces after it.
      const auto Src = Cmd.ModuleSource;
      const auto Pos =
          static_cast<size_t>(DefinitionKeyword.data() - Src.data());
      auto After = Pos + DefinitionKeyword.size();
      while (After < Src.size() && Src[After] == ' ') {
        ++After;
      }
      Script.OwnedStrings.push_back(std::string(Src.substr(0, Pos)) +
                                    std::string(Src.substr(After)));
      Cmd.ModuleSource = Script.OwnedStrings.back();
    }
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

      if (CT == NodeType::Id && !Cmd.ThreadName) {
        Cmd.ThreadName = identifier(Child);
        continue;
      }
      if (CT != NodeType::Sexpr) {
        reportError(Child, "unexpected token in thread");
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
      if (nodeType(Child) == NodeType::Id && !Cmd.ThreadName) {
        Cmd.ThreadName = identifier(Child);
      } else {
        reportError(Child, "unexpected token in wait");
      }
    }
    if (!Cmd.ThreadName) {
      reportError(N, "wait has no thread name");
    }
    return Cmd;
  }

  // --- Resolve the module source ---

  // A quote module holds WAT text, so the runner reads it as a text module.
  static void resolveModuleSource(ScriptCommand &Cmd) {
    if (Cmd.ModType == ModuleType::Quote) {
      Cmd.ModType = ModuleType::Text;
    }
  }

  // --- Helpers that decode the strings ---

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
    Script.OwnedStrings.push_back(std::move(*S));
    return Script.OwnedStrings.back();
  }

  // State
  WastScript Script;
  std::string_view SourceView;
  bool HasError = false;
};

// --- WastConverter::convert, which comes after the helpers ---

Expect<WastScript> WastConverter::convert(const Tree &T, std::string Source) {
  Script.Source = std::make_unique<std::string>(std::move(Source));
  SourceView = *Script.Source;

  // A syntax error can hide a command, so the script is not valid. A
  // malformed module of an assertion is in a string, and it does not make an
  // error here.
  Node Root = T.rootNode();
  if (Root.hasError()) {
    reportError(Root, "the script has a syntax error");
  }
  // The grammar skips an annotation as an extra, also between the commands,
  // so the id of each annotation needs a check here. Two tokens also need
  // whitespace between them, as in WAT. The text of a quoted module is one
  // string token, so its own tokens do not count here.
  Node Prev;
  Root.forEachLeaf([this, &Prev](Node Leaf, bool IsAnnotation) {
    if (IsAnnotation) {
      if (!WAT::checkAnnotation(Leaf.text(SourceView))) {
        reportError(Leaf, "malformed annotation");
      }
      return;
    }
    if (!Prev.isNull() && Prev.endByte() == Leaf.startByte() &&
        !WAT::checkAdjacentTokens(Prev.text(SourceView),
                                  Leaf.text(SourceView))) {
      reportError(Leaf, "two tokens need whitespace between them");
    }
    Prev = Leaf;
  });

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

  return std::move(Script);
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

  // The script follows the lexical rules of WAT, also between the commands.
  // The converter of a module checks only the text of the module. A string
  // can hold any valid UTF-8, so the malformed module text of a quoted module
  // does not fail here.
  if (auto Res = WAT::scanSourceCharacters(Source, nullptr); !Res) {
    spdlog::error("Illegal character or malformed UTF-8 in WAST file: {}"sv,
                  u8string(Path));
    return Unexpect(Res.error());
  }

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
