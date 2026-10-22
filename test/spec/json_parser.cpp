// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "json_parser.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "wat/wat_util.h"

#include "simdjson.h"

#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Wast {

using namespace std::literals;

// ---------------------------------------------------------------------------
// Type mapping helpers
// ---------------------------------------------------------------------------

static std::optional<TypeCode> stringToTypeCode(std::string_view S) {
  static constexpr std::pair<std::string_view, TypeCode> Types[] = {
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
  return WAT::lookupKeyword(Types, S);
}

static std::optional<CommandType> resolveCommand(std::string_view Name) {
  static constexpr std::pair<std::string_view, CommandType> Commands[] = {
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
  return WAT::lookupKeyword(Commands, Name);
}

// ---------------------------------------------------------------------------
// SIMD lane parser
// ---------------------------------------------------------------------------

// Values is a range of decimal lane strings, such as a JSON array or a vector.
template <typename T, typename RangeT>
static uint128_t parseSIMDLanes(const RangeT &Values) {
  constexpr size_t LaneCount = 16 / sizeof(T);
  assuming(LaneCount == Values.size());
  uint128_t V128 = 0;
  size_t I = 0;
  for (std::string_view X : Values) {
    WAT::setV128Lane(V128, I, static_cast<T>(std::stoull(std::string(X))));
    I++;
  }
  return V128;
}

// Give the size in bytes of a lane type, such as "f32".
static size_t laneSize(std::string_view LaneType) {
  static constexpr std::pair<std::string_view, size_t> Sizes[] = {
      {"i64"sv, 8}, {"f64"sv, 8}, {"i32"sv, 4},
      {"f32"sv, 4}, {"i16"sv, 2}, {"i8"sv, 1},
  };
  const auto Size = WAT::lookupKeyword(Sizes, LaneType);
  assuming(Size.has_value());
  return *Size;
}

// Build a V128 from the decimal lane strings Values with lanes of LaneSize
// bytes.
template <typename RangeT>
static uint128_t parseV128(size_t LaneSize, const RangeT &Values) {
  switch (LaneSize) {
  case 8:
    return parseSIMDLanes<uint64_t>(Values);
  case 4:
    return parseSIMDLanes<uint32_t>(Values);
  case 2:
    return parseSIMDLanes<uint16_t>(Values);
  case 1:
    return parseSIMDLanes<uint8_t>(Values);
  default:
    assumingUnreachable();
  }
}

// ---------------------------------------------------------------------------
// Parse the action arguments into a ValVariant vector and a ValType vector
// ---------------------------------------------------------------------------

static void parseValueList(const simdjson::dom::array &Args,
                           std::vector<ValVariant> &Vals,
                           std::vector<ValType> &Types) {
  Vals.reserve(Args.size());
  Types.reserve(Args.size());
  for (const simdjson::dom::object Element : Args) {
    std::string_view Type = Element["type"];
    simdjson::dom::element Value = Element["value"];
    if (Value.type() == simdjson::dom::element_type::ARRAY) {
      simdjson::dom::array ValueNodeArray = Value;
      std::string_view LaneType = Element["lane_type"];
      const uint128_t V128 = parseV128(laneSize(LaneType), ValueNodeArray);
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
          Vals.emplace_back(
              makeHostRef(*TC, std::stoul(std::string(ValueStr))));
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
    // The element has a type but no value. This is an opaque reference. A
    // bare "ref" matches every reference type.
    if (Type == "ref"sv) {
      R.Type = ValType(TypeCode::AnyRef);
    } else {
      auto TC = stringToTypeCode(Type);
      assuming(TC.has_value());
      R.Type = ValType(*TC);
    }
    R.Value = ValVariant(static_cast<uint64_t>(0));
    R.OpaqueRef = true;
    return R;
  }

  if (Value.type() == simdjson::dom::element_type::ARRAY) {
    // This is an expected V128 value.
    simdjson::dom::array ValueNodeArray = Value;
    std::string_view LaneType = Element["lane_type"];

    // Build the full shape string, for example "f32x4". It is the lane_type,
    // then "x", then 16/sizeof(lane).
    const size_t LaneSize = laneSize(LaneType);
    size_t LaneCount = 16 / LaneSize;
    std::string Shape(LaneType);
    Shape += "x" + std::to_string(LaneCount);

    R.Type = ValType(TypeCode::V128);
    R.V128Shape = std::move(Shape);

    // Scan the lanes for the NaN patterns. A NaN lane holds 0 in the value.
    R.V128LaneNaN.resize(LaneCount, Result::NaNPattern::None);

    bool HasNaN = false;
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

    // There is no NaN lane. Clear the vector to save space.
    if (!HasNaN) {
      R.V128LaneNaN.clear();
    }

    R.Value = ValVariant(parseV128(LaneSize, LaneValues));
    return R;
  }

  // Scalar string value.
  std::string_view ValueStr = Value;

  // Convert a bare "ref" type. It matches every reference type.
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

  // Look for the NaN patterns.
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

  // This is a typed null. Keep a concrete RefVariant of the expected heap
  // type, and keep AnyNullRef false. compareResult then selects the code for
  // each heap type. Only the bare "ref" null with no type matches every null
  // reference. The code before this point converts that null.
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
      return R;
    default:
      break;
    }
  }

  // These are the reference types with an opaque value, or with a value that
  // is not null. The code before this point converts a null value.
  switch (*TC) {
  case TypeCode::ExternRef:
  case TypeCode::AnyRef:
    R.Value = ValVariant(makeHostRef(*TC, std::stoul(std::string(ValueStr))));
    return R;
  case TypeCode::FuncRef:
    // This is an opaque function reference. Only the type is important, and
    // the value is not important.
    R.Value = ValVariant(static_cast<uint64_t>(0));
    R.OpaqueRef = true;
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
  for (const simdjson::dom::object Element : Args) {
    ResultOrEither ROE;
    ROE.Alternatives.push_back(parseOneExpected(Element));
    Results.push_back(std::move(ROE));
  }
  return Results;
}

// ---------------------------------------------------------------------------
// Parse an "either" array. The array is a flat list of the alternatives for
// one result position. The function returns one ResultOrEither.
// ---------------------------------------------------------------------------

static std::vector<ResultOrEither>
parseEithersList(const simdjson::dom::array &EitherArray) {
  ResultOrEither ROE;
  for (const simdjson::dom::object Element : EitherArray) {
    ROE.Alternatives.push_back(parseOneExpected(Element));
  }
  if (ROE.Alternatives.empty()) {
    return {};
  }
  return {std::move(ROE)};
}

// ---------------------------------------------------------------------------
// Take ownership of a string and return a string_view into OwnedStrings
// ---------------------------------------------------------------------------

static std::string_view ownStr(std::deque<std::string> &OwnedStrings,
                               std::string_view S) {
  OwnedStrings.emplace_back(S);
  return std::string_view(OwnedStrings.back());
}

// Take ownership of the optional string member Key of Obj. The result is empty
// if Obj has no such member.
static std::optional<std::string_view>
optStr(const simdjson::dom::object &Obj, std::string_view Key,
       std::deque<std::string> &OwnedStrings) {
  std::string_view S;
  if (Obj[Key].get(S)) {
    return std::nullopt;
  }
  return ownStr(OwnedStrings, S);
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

  Act.ModuleName = optStr(ActObj, "module"sv, OwnedStrings);

  std::string_view Field = ActObj["field"];
  Act.FieldName = std::string(Field);

  simdjson::dom::array Args;
  if (!ActObj["args"].get(Args)) {
    parseValueList(Args, Act.Args, Act.ArgTypes);
  }

  return Act;
}

// ---------------------------------------------------------------------------
// Resolve the module_type field into a ModuleType
// ---------------------------------------------------------------------------

static ModuleType resolveModType(const simdjson::dom::object &Obj) {
  std::string_view MT;
  if (!Obj["module_type"].get(MT) && MT == "text"sv) {
    return ModuleType::TextFile;
  }
  return ModuleType::BinaryFile;
}

// Convert one command. A thread command converts its sub-commands with the
// same function.
static Expect<ScriptCommand>
parseCommand(const simdjson::dom::object &Cmd, WastScript &Script,
             const std::filesystem::path &JsonPath) {
  // A command with no type field, or with a type that the runner does not
  // know, is an error. A skip hides the missing coverage behind a green
  // result.
  std::string_view TypeField;
  if (Cmd["type"].get(TypeField)) {
    spdlog::error("A command has no type field in {}"sv, u8string(JsonPath));
    return Unexpect(ErrCode::Value::IllegalGrammar);
  }

  auto CmdType = resolveCommand(TypeField);
  if (!CmdType.has_value()) {
    spdlog::error("Unknown command type '{}' in {}"sv, TypeField,
                  u8string(JsonPath));
    return Unexpect(ErrCode::Value::IllegalGrammar);
  }

  ScriptCommand SC;
  SC.Type = *CmdType;
  SC.Line = static_cast<uint32_t>(uint64_t(Cmd["line"]));

  switch (*CmdType) {
  case CommandType::Module:
  case CommandType::ModuleDefinition: {
    std::string_view FileName = Cmd["filename"];
    SC.ModuleSource = ownStr(Script.OwnedStrings, FileName);
    SC.ModType = resolveModType(Cmd);

    SC.ModuleName = optStr(Cmd, "name"sv, Script.OwnedStrings);
    break;
  }
  case CommandType::ModuleInstance: {
    SC.ModuleName = optStr(Cmd, "name"sv, Script.OwnedStrings);
    SC.DefinitionName = optStr(Cmd, "definition"sv, Script.OwnedStrings);
    break;
  }
  case CommandType::Register: {
    std::string_view AsName = Cmd["as"];
    SC.RegisterName = ownStr(Script.OwnedStrings, AsName);

    SC.ModuleName = optStr(Cmd, "name"sv, Script.OwnedStrings);
    break;
  }
  case CommandType::Action: {
    const simdjson::dom::object ActObj = Cmd["action"];
    SC.Act = parseAction(ActObj, Script.OwnedStrings);

    simdjson::dom::array Exp;
    if (!Cmd["expected"].get(Exp)) {
      SC.Expected = parseExpectedList(Exp);
    }
    break;
  }
  case CommandType::AssertReturn: {
    const simdjson::dom::object ActObj = Cmd["action"];
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
    // wast2json writes an assert_trap in two forms. The first form has an
    // "action" for an invoke trap or a get trap. The second form has a
    // module reference for an instantiation trap. Convert the two forms.
    simdjson::dom::object ActObj;
    if (!Cmd["action"].get(ActObj)) {
      SC.Act = parseAction(ActObj, Script.OwnedStrings);
    } else {
      std::string_view FileName;
      if (!Cmd["filename"].get(FileName)) {
        SC.ModuleSource = ownStr(Script.OwnedStrings, FileName);
        SC.ModType = resolveModType(Cmd);
      }
    }

    SC.ExpectedMessage =
        optStr(Cmd, "text"sv, Script.OwnedStrings).value_or(""sv);
    break;
  }
  case CommandType::AssertExhaustion: {
    const simdjson::dom::object ActObj = Cmd["action"];
    SC.Act = parseAction(ActObj, Script.OwnedStrings);

    SC.ExpectedMessage =
        optStr(Cmd, "text"sv, Script.OwnedStrings).value_or(""sv);
    break;
  }
  case CommandType::AssertMalformed:
  case CommandType::AssertInvalid:
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
    const simdjson::dom::object ActObj = Cmd["action"];
    SC.Act = parseAction(ActObj, Script.OwnedStrings);
    break;
  }
  case CommandType::Thread: {
    std::string_view ThreadName = Cmd["name"];
    SC.ThreadName = ownStr(Script.OwnedStrings, ThreadName);

    simdjson::dom::array ThreadCmds = Cmd["commands"];

    // Parse shared modules.
    simdjson::dom::array SharedArray;
    if (!Cmd["shared"].get(SharedArray)) {
      for (const simdjson::dom::object SharedEntry : SharedArray) {
        std::string_view ModRef = SharedEntry["module"];
        // Store the module name. The executor makes the alias when it runs
        // the command.
        SC.SharedModules.emplace_back(ModRef);
      }
    }

    for (const simdjson::dom::object SubCmd : ThreadCmds) {
      EXPECTED_TRY(auto Sub, parseCommand(SubCmd, Script, JsonPath));
      SC.SubCommands.push_back(std::move(Sub));
    }
    break;
  }
  case CommandType::Wait: {
    std::string_view ThreadName = Cmd["thread"];
    SC.ThreadName = ownStr(Script.OwnedStrings, ThreadName);
    break;
  }
  }

  return SC;
}

// ---------------------------------------------------------------------------
// The main entry. It parses a JSON file that wast2json wrote
// ---------------------------------------------------------------------------

Expect<WastScript> parseJson(const std::filesystem::path &JsonPath) {
  WastScript Script;

  simdjson::dom::parser Parser;
  simdjson::dom::element Doc;
  auto Err = Parser.load(JsonPath.string()).get(Doc);
  if (Err) {
    spdlog::error("Failed to load JSON: {}"sv, u8string(JsonPath));
    return Unexpect(ErrCode::Value::IllegalPath);
  }

  simdjson::dom::array CmdArray;
  if (Doc["commands"].get(CmdArray)) {
    spdlog::error("No 'commands' array in JSON: {}"sv, u8string(JsonPath));
    return Unexpect(ErrCode::Value::IllegalGrammar);
  }

  Script.Commands.reserve(CmdArray.size());

  // The implicit conversions of simdjson, std::stoul, and std::stoull throw on
  // malformed or unexpected input. Catch the exception here and report it, so
  // that the exception does not escape from the test binary. An unknown value
  // type or a wrong lane count is not caught: the parser assumes that the
  // JSON files come from the wast2json tool.
  try {
    for (const simdjson::dom::object Cmd : CmdArray) {
      EXPECTED_TRY(auto SC, parseCommand(Cmd, Script, JsonPath));
      Script.Commands.push_back(std::move(SC));
    }
  } catch (const std::exception &E) {
    spdlog::error("Failed to parse JSON commands in {}: {}"sv,
                  u8string(JsonPath), E.what());
    return Unexpect(ErrCode::Value::IllegalGrammar);
  }

  return Script;
}

} // namespace Wast
} // namespace WasmEdge
