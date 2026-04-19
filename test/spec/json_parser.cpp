// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "json_parser.h"
#include "common/hash.h"
#include "common/spdlog.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace WasmEdge {
namespace Wast {

using namespace std::literals;

// ---------------------------------------------------------------------------
// Type mapping helpers
// ---------------------------------------------------------------------------

static std::optional<TypeCode> stringToTypeCode(std::string_view S) {
  static const std::unordered_map<std::string_view, TypeCode, Hash::Hash> Map =
      {
          {"i32"sv, TypeCode::I32},
          {"i64"sv, TypeCode::I64},
          {"f32"sv, TypeCode::F32},
          {"f64"sv, TypeCode::F64},
          {"v128"sv, TypeCode::V128},
          {"funcref"sv, TypeCode::FuncRef},
          {"externref"sv, TypeCode::ExternRef},
          {"anyref"sv, TypeCode::AnyRef},
          {"eqref"sv, TypeCode::EqRef},
          {"i31ref"sv, TypeCode::I31Ref},
          {"structref"sv, TypeCode::StructRef},
          {"arrayref"sv, TypeCode::ArrayRef},
          {"nullref"sv, TypeCode::NullRef},
          {"nullfuncref"sv, TypeCode::NullFuncRef},
          {"nullexternref"sv, TypeCode::NullExternRef},
          {"exnref"sv, TypeCode::ExnRef},
          {"nullexnref"sv, TypeCode::NullExnRef},
      };
  auto It = Map.find(S);
  return It != Map.end() ? std::optional(It->second) : std::nullopt;
}

static std::optional<CommandType> resolveCommand(std::string_view Name) {
  static const std::unordered_map<std::string_view, CommandType, Hash::Hash>
      Map = {
          {"module"sv, CommandType::Module},
          {"module_definition"sv, CommandType::ModuleDefinition},
          {"module_instance"sv, CommandType::ModuleInstance},
          {"action"sv, CommandType::Action},
          {"register"sv, CommandType::Register},
          {"assert_return"sv, CommandType::AssertReturn},
          {"assert_trap"sv, CommandType::AssertTrap},
          {"assert_exhaustion"sv, CommandType::AssertExhaustion},
          {"assert_malformed"sv, CommandType::AssertMalformed},
          {"assert_invalid"sv, CommandType::AssertInvalid},
          {"assert_unlinkable"sv, CommandType::AssertUnlinkable},
          {"assert_uninstantiable"sv, CommandType::AssertUninstantiable},
          {"assert_exception"sv, CommandType::AssertException},
          {"thread"sv, CommandType::Thread},
          {"wait"sv, CommandType::Wait},
      };
  auto It = Map.find(Name);
  return It != Map.end() ? std::optional(It->second) : std::nullopt;
}

// ---------------------------------------------------------------------------
// SIMD lane parser
// ---------------------------------------------------------------------------

template <typename T, size_t N = sizeof(T)>
static void parseSIMDLanes(uint128_t &V128,
                           const simdjson::dom::array &ValueNodeArray) {
  assuming(16 / N == ValueNodeArray.size());
  T V[16 / N];
  size_t I = 0;
  for (std::string_view X : ValueNodeArray) {
    V[I] = static_cast<T>(std::stoull(std::string(X)));
    I++;
  }
  if constexpr (Endian::native == Endian::big) {
    std::reverse(V, V + 16 / N);
  }
  std::memcpy(&V128, &V, 16);
}

enum class LaneSizeKind { Size8, Size4, Size2, Size1, Unknown };
static const auto &LaneSizeKindMap() {
  static const std::unordered_map<std::string_view, LaneSizeKind, Hash::Hash>
      Map = {
          {"i64"sv, LaneSizeKind::Size8}, {"f64"sv, LaneSizeKind::Size8},
          {"i32"sv, LaneSizeKind::Size4}, {"f32"sv, LaneSizeKind::Size4},
          {"i16"sv, LaneSizeKind::Size2}, {"i8"sv, LaneSizeKind::Size1},
      };
  return Map;
}

// ---------------------------------------------------------------------------
// Parse action args into ValVariant + ValType vectors
// ---------------------------------------------------------------------------

static void parseValueList(const simdjson::dom::array &Args,
                           std::vector<ValVariant> &Vals,
                           std::vector<ValType> &Types) {
  Vals.reserve(Args.size());
  Types.reserve(Args.size());
  for (const simdjson::dom::object &Element : Args) {
    std::string_view Type = Element["type"];
    simdjson::dom::element Value = Element["value"];
    if (Value.type() == simdjson::dom::element_type::ARRAY) {
      simdjson::dom::array ValueNodeArray = Value;
      uint128_t V128;
      std::string_view LaneType = Element["lane_type"];
      auto LaneIt = LaneSizeKindMap().find(LaneType);
      auto LSK = (LaneIt != LaneSizeKindMap().end()) ? LaneIt->second
                                                     : LaneSizeKind::Unknown;
      switch (LSK) {
      case LaneSizeKind::Size8:
        parseSIMDLanes<uint64_t>(V128, ValueNodeArray);
        break;
      case LaneSizeKind::Size4:
        parseSIMDLanes<uint32_t>(V128, ValueNodeArray);
        break;
      case LaneSizeKind::Size2:
        parseSIMDLanes<uint16_t>(V128, ValueNodeArray);
        break;
      case LaneSizeKind::Size1:
        parseSIMDLanes<uint8_t>(V128, ValueNodeArray);
        break;
      default:
        assumingUnreachable();
      }
      Vals.emplace_back(V128);
      Types.emplace_back(TypeCode::V128);
    } else if (Value.type() == simdjson::dom::element_type::STRING) {
      std::string_view ValueStr = Value;
      auto TC = stringToTypeCode(Type);
      assuming(TC.has_value());
      switch (*TC) {
      case TypeCode::ExternRef:
      case TypeCode::AnyRef:
        if (ValueStr == "null"sv) {
          Vals.emplace_back(RefVariant(*TC));
        } else {
          Vals.emplace_back(RefVariant(
              *TC, reinterpret_cast<void *>(std::stoul(std::string(ValueStr)) +
                                            0x100000000ULL)));
        }
        break;
      case TypeCode::FuncRef:
        if (ValueStr == "null"sv) {
          Vals.emplace_back(RefVariant(TypeCode::FuncRef));
        } else {
          assumingUnreachable();
        }
        break;
      case TypeCode::I32:
      case TypeCode::F32:
        Vals.emplace_back(
            static_cast<uint32_t>(std::stoul(std::string(ValueStr))));
        break;
      case TypeCode::I64:
      case TypeCode::F64:
        Vals.emplace_back(
            static_cast<uint64_t>(std::stoull(std::string(ValueStr))));
        break;
      default:
        assumingUnreachable();
      }
      Types.emplace_back(*TC);
    } else {
      assumingUnreachable();
    }
  }
}

// ---------------------------------------------------------------------------
// Parse a single expected element into a Result
// ---------------------------------------------------------------------------

static Result parseOneExpected(const simdjson::dom::object &Element) {
  std::string_view Type = Element["type"];
  Result R;

  simdjson::dom::element Value;
  auto NoValue = Element["value"].get(Value);
  if (NoValue) {
    // Only type, no value — opaque reference check.
    if (Type == "ref"sv) {
      // Bare "ref" with no value: opaque, matches any reference type.
      R.Type = ValType(TypeCode::AnyRef);
      R.Value = ValVariant(static_cast<uint64_t>(0));
      R.OpaqueRef = true;
      return R;
    }
    auto TC = stringToTypeCode(Type);
    assuming(TC.has_value());
    R.Type = ValType(*TC);
    R.Value = ValVariant(static_cast<uint64_t>(0));
    R.OpaqueRef = true;
    return R;
  }

  if (Value.type() == simdjson::dom::element_type::ARRAY) {
    // V128 expected value
    simdjson::dom::array ValueNodeArray = Value;
    std::string_view LaneType = Element["lane_type"];

    // Determine the full shape string, e.g. "f32x4"
    // The JSON type is "v128", lane_type is e.g. "f32", and lane count is
    // 16/sizeof(lane).
    size_t LaneSize = 0;
    auto LaneIt = LaneSizeKindMap().find(LaneType);
    auto LSK = (LaneIt != LaneSizeKindMap().end()) ? LaneIt->second
                                                   : LaneSizeKind::Unknown;
    switch (LSK) {
    case LaneSizeKind::Size8:
      LaneSize = 8;
      break;
    case LaneSizeKind::Size4:
      LaneSize = 4;
      break;
    case LaneSizeKind::Size2:
      LaneSize = 2;
      break;
    case LaneSizeKind::Size1:
      LaneSize = 1;
      break;
    default:
      assumingUnreachable();
    }
    size_t LaneCount = 16 / LaneSize;
    std::string Shape(LaneType);
    Shape += "x" + std::to_string(LaneCount);

    R.Type = ValType(TypeCode::V128);
    R.V128Shape = std::move(Shape);

    // Check per-lane for NaN patterns, and build the V128 value.
    // For lanes that are NaN patterns, store 0 in that lane.
    R.V128LaneNaN.resize(LaneCount, Result::NaNPattern::None);

    bool HasNaN = false;
    // We need to build the V128 value. Parse lane values, replacing NaN
    // patterns with 0.
    std::vector<std::string> LaneValues;
    LaneValues.reserve(LaneCount);
    size_t Idx = 0;
    for (std::string_view X : ValueNodeArray) {
      if (X == "nan:canonical"sv) {
        R.V128LaneNaN[Idx] = Result::NaNPattern::Canonical;
        HasNaN = true;
        LaneValues.emplace_back("0");
      } else if (X == "nan:arithmetic"sv) {
        R.V128LaneNaN[Idx] = Result::NaNPattern::Arithmetic;
        HasNaN = true;
        LaneValues.emplace_back("0");
      } else {
        LaneValues.emplace_back(X);
      }
      Idx++;
    }

    // If no NaN at all, clear the vector to save space.
    if (!HasNaN) {
      R.V128LaneNaN.clear();
    }

    // Build V128 from lane values.
    uint128_t V128Val;
    // Create a temporary simdjson-like structure is complex; instead just
    // parse manually.
    switch (LaneSize) {
    case 8: {
      assuming(LaneCount == 2);
      uint64_t V[2];
      for (size_t I = 0; I < 2; I++) {
        V[I] = static_cast<uint64_t>(std::stoull(LaneValues[I]));
      }
      if constexpr (Endian::native == Endian::big) {
        std::reverse(V, V + 2);
      }
      std::memcpy(&V128Val, &V, 16);
      break;
    }
    case 4: {
      assuming(LaneCount == 4);
      uint32_t V[4];
      for (size_t I = 0; I < 4; I++) {
        V[I] = static_cast<uint32_t>(std::stoul(LaneValues[I]));
      }
      if constexpr (Endian::native == Endian::big) {
        std::reverse(V, V + 4);
      }
      std::memcpy(&V128Val, &V, 16);
      break;
    }
    case 2: {
      assuming(LaneCount == 8);
      uint16_t V[8];
      for (size_t I = 0; I < 8; I++) {
        V[I] = static_cast<uint16_t>(std::stoul(LaneValues[I]));
      }
      if constexpr (Endian::native == Endian::big) {
        std::reverse(V, V + 8);
      }
      std::memcpy(&V128Val, &V, 16);
      break;
    }
    case 1: {
      assuming(LaneCount == 16);
      uint8_t V[16];
      for (size_t I = 0; I < 16; I++) {
        V[I] = static_cast<uint8_t>(std::stoul(LaneValues[I]));
      }
      if constexpr (Endian::native == Endian::big) {
        std::reverse(V, V + 16);
      }
      std::memcpy(&V128Val, &V, 16);
      break;
    }
    default:
      assumingUnreachable();
    }
    R.Value = ValVariant(V128Val);
    return R;
  }

  // Scalar string value.
  std::string_view ValueStr = Value;

  // Handle bare "ref" type (matches any reference type).
  if (Type == "ref"sv) {
    R.Type = ValType(TypeCode::AnyRef);
    if (ValueStr == "null"sv) {
      R.AnyNullRef = true;
      R.Value = ValVariant(RefVariant(TypeCode::AnyRef));
    } else {
      R.OpaqueRef = true;
      R.Value = ValVariant(static_cast<uint64_t>(0));
    }
    return R;
  }

  auto TC = stringToTypeCode(Type);
  assuming(TC.has_value());
  R.Type = ValType(*TC);

  // Check for NaN patterns.
  if (ValueStr == "nan:canonical"sv) {
    R.NaN = Result::NaNPattern::Canonical;
    R.Value = ValVariant(static_cast<uint32_t>(0));
    return R;
  }
  if (ValueStr == "nan:arithmetic"sv) {
    R.NaN = Result::NaNPattern::Arithmetic;
    R.Value = ValVariant(static_cast<uint32_t>(0));
    return R;
  }

  // Check for null references.
  if (ValueStr == "null"sv) {
    switch (*TC) {
    case TypeCode::FuncRef:
    case TypeCode::ExternRef:
    case TypeCode::AnyRef:
    case TypeCode::EqRef:
    case TypeCode::I31Ref:
    case TypeCode::StructRef:
    case TypeCode::ArrayRef:
    case TypeCode::NullRef:
    case TypeCode::NullFuncRef:
    case TypeCode::NullExternRef:
    case TypeCode::ExnRef:
    case TypeCode::NullExnRef:
      R.Value = ValVariant(RefVariant(*TC));
      R.AnyNullRef = true;
      return R;
    default:
      break;
    }
  }

  // Check for reference types with opaque or non-null values.
  switch (*TC) {
  case TypeCode::ExternRef:
  case TypeCode::AnyRef:
    if (ValueStr == "null"sv) {
      R.Value = ValVariant(RefVariant(*TC));
    } else {
      R.Value = ValVariant(RefVariant(
          *TC, reinterpret_cast<void *>(std::stoul(std::string(ValueStr)) +
                                        0x100000000ULL)));
    }
    return R;
  case TypeCode::FuncRef:
    if (ValueStr == "null"sv) {
      R.Value = ValVariant(RefVariant(TypeCode::FuncRef));
    } else {
      // Opaque function ref — we only check the type, not the value.
      R.Value = ValVariant(static_cast<uint64_t>(0));
      R.OpaqueRef = true;
    }
    return R;
  case TypeCode::EqRef:
  case TypeCode::I31Ref:
  case TypeCode::StructRef:
  case TypeCode::ArrayRef:
  case TypeCode::NullRef:
  case TypeCode::NullFuncRef:
  case TypeCode::NullExternRef:
  case TypeCode::ExnRef:
  case TypeCode::NullExnRef:
    // These are opaque reference types in the JSON format.
    R.Value = ValVariant(static_cast<uint64_t>(0));
    R.OpaqueRef = true;
    return R;
  default:
    break;
  }

  // Numeric types.
  switch (*TC) {
  case TypeCode::I32:
  case TypeCode::F32:
    R.Value =
        ValVariant(static_cast<uint32_t>(std::stoul(std::string(ValueStr))));
    break;
  case TypeCode::I64:
  case TypeCode::F64:
    R.Value =
        ValVariant(static_cast<uint64_t>(std::stoull(std::string(ValueStr))));
    break;
  default:
    assumingUnreachable();
  }
  return R;
}

// ---------------------------------------------------------------------------
// Parse expected list into vector of ResultOrEither (single alternative each)
// ---------------------------------------------------------------------------

static std::vector<ResultOrEither>
parseExpectedList(const simdjson::dom::array &Args) {
  std::vector<ResultOrEither> Results;
  Results.reserve(Args.size());
  for (const simdjson::dom::object &Element : Args) {
    ResultOrEither ROE;
    ROE.Alternatives.push_back(parseOneExpected(Element));
    Results.push_back(std::move(ROE));
  }
  return Results;
}

// ---------------------------------------------------------------------------
// Parse "either" array: each element is one alternative expected result.
// In wast2json output, "either" is a flat array of expected values — each is
// an alternative for a single result position.
// Return a single ResultOrEither containing all alternatives.
// ---------------------------------------------------------------------------

static std::vector<ResultOrEither>
parseEithersList(const simdjson::dom::array &EitherArray) {
  ResultOrEither ROE;
  for (const simdjson::dom::object &Element : EitherArray) {
    ROE.Alternatives.push_back(parseOneExpected(Element));
  }
  if (ROE.Alternatives.empty()) {
    return {};
  }
  return {std::move(ROE)};
}

// ---------------------------------------------------------------------------
// Parse an "action" JSON object into a Wast::Action
// ---------------------------------------------------------------------------

static Action parseAction(const simdjson::dom::object &ActObj,
                          std::deque<std::string> &OwnedStrings) {
  Action Act;

  std::string_view ActType = ActObj["type"];
  if (ActType == "invoke"sv) {
    Act.Type = ActionType::Invoke;
  } else if (ActType == "get"sv) {
    Act.Type = ActionType::Get;
  } else {
    assumingUnreachable();
  }

  std::string_view ModName;
  if (!ActObj["module"].get(ModName)) {
    OwnedStrings.emplace_back(ModName);
    Act.ModuleName = std::string_view(OwnedStrings.back());
  }

  std::string_view Field = ActObj["field"];
  Act.FieldName = std::string(Field);

  simdjson::dom::array Args;
  if (!ActObj["args"].get(Args)) {
    parseValueList(Args, Act.Args, Act.ArgTypes);
  }

  return Act;
}

// ---------------------------------------------------------------------------
// Helper: own a string and return a string_view into OwnedStrings
// ---------------------------------------------------------------------------

static ModuleType resolveModType(const simdjson::dom::object &Obj) {
  std::string_view MT;
  if (!Obj["module_type"].get(MT) && MT == "text"sv) {
    return ModuleType::TextFile;
  }
  return ModuleType::BinaryFile;
}

static std::string_view ownStr(std::deque<std::string> &OwnedStrings,
                               std::string_view S) {
  OwnedStrings.emplace_back(S);
  return std::string_view(OwnedStrings.back());
}

// ---------------------------------------------------------------------------
// Recursive thread sub-command parser
// ---------------------------------------------------------------------------

static void parseThreadSubCommands(const simdjson::dom::array &Cmds,
                                   std::vector<ScriptCommand> &Out,
                                   JsonScript &Script) {
  for (const simdjson::dom::object &SubCmd : Cmds) {
    std::string_view SubType;
    if (SubCmd["type"].get(SubType)) {
      continue;
    }
    auto SubCmdType = resolveCommand(SubType);
    if (!SubCmdType.has_value()) {
      continue;
    }

    ScriptCommand Sub;
    Sub.Type = *SubCmdType;
    Sub.Line = static_cast<uint32_t>(uint64_t(SubCmd["line"]));

    switch (*SubCmdType) {
    case CommandType::Module: {
      std::string_view FN = SubCmd["filename"];
      Sub.ModuleSource = ownStr(Script.OwnedStrings, FN);
      Sub.ModType = resolveModType(SubCmd);
      std::string_view N;
      if (!SubCmd["name"].get(N)) {
        Sub.ModuleName = ownStr(Script.OwnedStrings, N);
      }
      break;
    }
    case CommandType::Register: {
      std::string_view AsN = SubCmd["as"];
      Sub.RegisterName = ownStr(Script.OwnedStrings, AsN);
      std::string_view N;
      if (!SubCmd["name"].get(N)) {
        Sub.ModuleName = ownStr(Script.OwnedStrings, N);
      }
      break;
    }
    case CommandType::Action:
    case CommandType::AssertReturn: {
      const simdjson::dom::object &A = SubCmd["action"];
      Sub.Act = parseAction(A, Script.OwnedStrings);
      simdjson::dom::array E, Ei;
      if (!SubCmd["expected"].get(E)) {
        Sub.Expected = parseExpectedList(E);
      } else if (!SubCmd["either"].get(Ei)) {
        Sub.Expected = parseEithersList(Ei);
      }
      break;
    }
    case CommandType::AssertTrap: {
      simdjson::dom::object ActObj;
      if (!SubCmd["action"].get(ActObj)) {
        Sub.Act = parseAction(ActObj, Script.OwnedStrings);
      }
      std::string_view FN;
      if (!SubCmd["filename"].get(FN)) {
        Sub.ModuleSource = ownStr(Script.OwnedStrings, FN);
        Sub.ModType = resolveModType(SubCmd);
      }
      std::string_view T;
      if (!SubCmd["text"].get(T)) {
        Sub.ExpectedMessage = ownStr(Script.OwnedStrings, T);
      }
      break;
    }
    case CommandType::AssertMalformed:
    case CommandType::AssertInvalid:
    case CommandType::AssertUnlinkable:
    case CommandType::AssertUninstantiable: {
      std::string_view FN;
      if (!SubCmd["filename"].get(FN)) {
        Sub.ModuleSource = ownStr(Script.OwnedStrings, FN);
        Sub.ModType = resolveModType(SubCmd);
      }
      std::string_view T;
      if (!SubCmd["text"].get(T)) {
        Sub.ExpectedMessage = ownStr(Script.OwnedStrings, T);
      }
      break;
    }
    case CommandType::Thread: {
      std::string_view N;
      if (!SubCmd["name"].get(N)) {
        Sub.ModuleName = ownStr(Script.OwnedStrings, N);
      }
      simdjson::dom::array SharedArr;
      if (!SubCmd["shared"].get(SharedArr)) {
        for (const simdjson::dom::object &SE : SharedArr) {
          std::string_view MR = SE["module"];
          Sub.SharedModules.emplace_back(std::string(MR), std::string());
        }
      }
      simdjson::dom::array NestedCmds = SubCmd["commands"];
      parseThreadSubCommands(NestedCmds, Sub.SubCommands, Script);
      break;
    }
    case CommandType::Wait: {
      std::string_view WN = SubCmd["thread"];
      Sub.ThreadName = ownStr(Script.OwnedStrings, WN);
      break;
    }
    default:
      break;
    }

    Out.push_back(std::move(Sub));
  }
}

// ---------------------------------------------------------------------------
// Main entry: parse a JSON file produced by wast2json
// ---------------------------------------------------------------------------

Expect<JsonScript> parseJson(const std::filesystem::path &JsonPath) {
  JsonScript Script;

  auto Err = Script.Parser.load(JsonPath.string()).get(Script.Doc);
  if (Err) {
    spdlog::error("Failed to load JSON: {}"sv, JsonPath.string());
    return Unexpect(ErrCode::Value::IllegalPath);
  }

  simdjson::dom::array CmdArray;
  if (Script.Doc["commands"].get(CmdArray)) {
    spdlog::error("No 'commands' array in JSON: {}"sv, JsonPath.string());
    return Unexpect(ErrCode::Value::IllegalPath);
  }

  Script.Commands.reserve(CmdArray.size());

  for (const simdjson::dom::object &Cmd : CmdArray) {
    std::string_view TypeField;
    if (Cmd["type"].get(TypeField)) {
      continue; // Skip commands without type field.
    }

    auto CmdType = resolveCommand(TypeField);
    if (!CmdType.has_value()) {
      spdlog::warn("Unknown command type '{}', skipping"sv, TypeField);
      continue;
    }

    ScriptCommand SC;
    SC.Type = *CmdType;
    SC.Line = static_cast<uint32_t>(uint64_t(Cmd["line"]));

    switch (*CmdType) {
    case CommandType::Module: {
      std::string_view FileName = Cmd["filename"];
      SC.ModuleSource = ownStr(Script.OwnedStrings, FileName);
      SC.ModType = resolveModType(Cmd);

      std::string_view Name;
      if (!Cmd["name"].get(Name)) {
        SC.ModuleName = ownStr(Script.OwnedStrings, Name);
      }
      break;
    }
    case CommandType::ModuleDefinition: {
      std::string_view FileName = Cmd["filename"];
      SC.ModuleSource = ownStr(Script.OwnedStrings, FileName);
      SC.ModType = resolveModType(Cmd);

      std::string_view Name;
      if (!Cmd["name"].get(Name)) {
        SC.ModuleName = ownStr(Script.OwnedStrings, Name);
      }
      break;
    }
    case CommandType::ModuleInstance: {
      std::string_view ModName;
      if (!Cmd["name"].get(ModName)) {
        SC.ModuleName = ownStr(Script.OwnedStrings, ModName);
      }
      std::string_view DefName;
      if (!Cmd["definition"].get(DefName)) {
        SC.DefinitionName = ownStr(Script.OwnedStrings, DefName);
      }
      break;
    }
    case CommandType::Register: {
      std::string_view AsName = Cmd["as"];
      SC.RegisterName = ownStr(Script.OwnedStrings, AsName);

      std::string_view Name;
      if (!Cmd["name"].get(Name)) {
        SC.ModuleName = ownStr(Script.OwnedStrings, Name);
      }
      break;
    }
    case CommandType::Action: {
      const simdjson::dom::object &ActObj = Cmd["action"];
      SC.Act = parseAction(ActObj, Script.OwnedStrings);

      simdjson::dom::array Exp;
      if (!Cmd["expected"].get(Exp)) {
        SC.Expected = parseExpectedList(Exp);
      }
      break;
    }
    case CommandType::AssertReturn: {
      const simdjson::dom::object &ActObj = Cmd["action"];
      SC.Act = parseAction(ActObj, Script.OwnedStrings);

      simdjson::dom::array Exp, Either;
      if (!Cmd["expected"].get(Exp)) {
        SC.Expected = parseExpectedList(Exp);
      } else if (!Cmd["either"].get(Either)) {
        SC.Expected = parseEithersList(Either);
      }
      break;
    }
    case CommandType::AssertTrap: {
      const simdjson::dom::object &ActObj = Cmd["action"];
      SC.Act = parseAction(ActObj, Script.OwnedStrings);

      std::string_view Text = Cmd["text"];
      SC.ExpectedMessage = ownStr(Script.OwnedStrings, Text);
      break;
    }
    case CommandType::AssertExhaustion: {
      const simdjson::dom::object &ActObj = Cmd["action"];
      SC.Act = parseAction(ActObj, Script.OwnedStrings);

      std::string_view Text;
      if (!Cmd["text"].get(Text)) {
        SC.ExpectedMessage = ownStr(Script.OwnedStrings, Text);
      }
      break;
    }
    case CommandType::AssertMalformed: {
      std::string_view FileName = Cmd["filename"];
      SC.ModuleSource = ownStr(Script.OwnedStrings, FileName);
      SC.ModType = resolveModType(Cmd);

      std::string_view Text = Cmd["text"];
      SC.ExpectedMessage = ownStr(Script.OwnedStrings, Text);
      break;
    }
    case CommandType::AssertInvalid: {
      std::string_view FileName = Cmd["filename"];
      SC.ModuleSource = ownStr(Script.OwnedStrings, FileName);
      SC.ModType = resolveModType(Cmd);

      std::string_view Text = Cmd["text"];
      SC.ExpectedMessage = ownStr(Script.OwnedStrings, Text);
      break;
    }
    case CommandType::AssertUnlinkable:
    case CommandType::AssertUninstantiable: {
      std::string_view FileName = Cmd["filename"];
      SC.ModuleSource = ownStr(Script.OwnedStrings, FileName);
      SC.ModType = resolveModType(Cmd);

      std::string_view Text = Cmd["text"];
      SC.ExpectedMessage = ownStr(Script.OwnedStrings, Text);
      break;
    }
    case CommandType::AssertException: {
      const simdjson::dom::object &ActObj = Cmd["action"];
      SC.Act = parseAction(ActObj, Script.OwnedStrings);
      break;
    }
    case CommandType::Thread: {
      std::string_view ThreadName = Cmd["name"];
      SC.ModuleName = ownStr(Script.OwnedStrings, ThreadName);

      simdjson::dom::array ThreadCmds = Cmd["commands"];

      // Parse shared modules.
      simdjson::dom::array SharedArray;
      if (!Cmd["shared"].get(SharedArray)) {
        for (const simdjson::dom::object &SharedEntry : SharedArray) {
          std::string_view ModRef = SharedEntry["module"];
          // For now, store module ref as the pair; aliasing is handled at
          // execution time.
          SC.SharedModules.emplace_back(std::string(ModRef), std::string());
        }
      }

      parseThreadSubCommands(ThreadCmds, SC.SubCommands, Script);
      break;
    }
    case CommandType::Wait: {
      std::string_view ThreadName = Cmd["thread"];
      SC.ThreadName = ownStr(Script.OwnedStrings, ThreadName);
      break;
    }
    default:
      break;
    }

    Script.Commands.push_back(std::move(SC));
  }

  return Script;
}

} // namespace Wast
} // namespace WasmEdge
