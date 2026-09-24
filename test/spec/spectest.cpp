// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/spec/spectest.cpp - Wasm test suites ----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file parses and runs tests of Wasm test suites extracted by wast2json.
/// Test Suites: https://github.com/WebAssembly/testsuite
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#include "spectest.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/hash.h"
#include "common/spdlog.h"

#include "simdjson.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>
#include <variant>

namespace {

using namespace std::literals;
using namespace WasmEdge;

// Preprocessing for set up aliasing.
void resolveRegister(std::map<std::string, std::string> &Alias,
                     simdjson::dom::array &CmdArray) {
  std::string_view OrgName;
  uint64_t LastModLine = 0;
  for (const simdjson::dom::object Cmd : CmdArray) {
    std::string_view CmdType = Cmd["type"];
    if (CmdType == "module"sv) {
      // Record last module in order
      if (Cmd["name"].get(OrgName)) {
        OrgName = {};
      }
      LastModLine = Cmd["line"];
    } else if (CmdType == "register"sv) {
      std::string_view NewNameStr = Cmd["as"];
      std::string_view Value;
      if (!Cmd["name"].get(Value)) {
        // Register command records the original name. Set aliasing.
        Alias.emplace(std::string(Value), std::string(NewNameStr));
      } else {
        // Register command does not record the original name. Get name from the
        // module.
        if (OrgName.empty()) {
          // Module has no origin name. Alias to the latest anonymous module.
          Alias.emplace(std::to_string(LastModLine), NewNameStr);
        } else {
          // Module has origin name. Replace to aliased one.
          Alias.emplace(std::string(OrgName), NewNameStr);
        }
      }
    }
  }
}

SpecTest::CommandID resolveCommand(std::string_view Name) {
  static const std::unordered_map<std::string_view, SpecTest::CommandID,
                                  Hash::Hash>
      CommandMapping = {
          {"module"sv, SpecTest::CommandID::Module},
          {"module_definition"sv, SpecTest::CommandID::ModuleDefinition},
          {"module_instance"sv, SpecTest::CommandID::ModuleInstance},
          {"action"sv, SpecTest::CommandID::Action},
          {"register"sv, SpecTest::CommandID::Register},
          {"assert_return"sv, SpecTest::CommandID::AssertReturn},
          {"assert_trap"sv, SpecTest::CommandID::AssertTrap},
          {"assert_exhaustion"sv, SpecTest::CommandID::AssertExhaustion},
          {"assert_malformed"sv, SpecTest::CommandID::AssertMalformed},
          {"assert_invalid"sv, SpecTest::CommandID::AssertInvalid},
          {"assert_unlinkable"sv, SpecTest::CommandID::AssertUnlinkable},
          {"assert_uninstantiable"sv,
           SpecTest::CommandID::AssertUninstantiable},
          {"assert_exception"sv, SpecTest::CommandID::AssertException},
          {"thread"sv, SpecTest::CommandID::Thread},
          {"wait"sv, SpecTest::CommandID::Wait},
      };
  if (auto Iter = CommandMapping.find(Name); Iter != CommandMapping.end()) {
    return Iter->second;
  }
  return SpecTest::CommandID::Unknown;
}

template <typename T, size_t N = sizeof(T)>
static void parseSIMDLanes(WasmEdge::uint128_t &V128,
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

// The first code point of a UTF-8 string: a `char` constant of the json.
uint32_t decodeCodepoint(std::string_view Str) {
  if (Str.empty()) {
    return 0;
  }
  const auto Lead = static_cast<uint8_t>(Str[0]);
  auto Tail = [&Str](size_t I) {
    return static_cast<uint32_t>(static_cast<uint8_t>(Str[I]) & 0x3FU);
  };
  if (Lead < 0x80) {
    return Lead;
  }
  if ((Lead & 0xE0U) == 0xC0U && Str.size() >= 2) {
    return ((Lead & 0x1FU) << 6) | Tail(1);
  }
  if ((Lead & 0xF0U) == 0xE0U && Str.size() >= 3) {
    return ((Lead & 0x0FU) << 12) | (Tail(1) << 6) | Tail(2);
  }
  if ((Lead & 0xF8U) == 0xF0U && Str.size() >= 4) {
    return ((Lead & 0x07U) << 18) | (Tail(1) << 12) | (Tail(2) << 6) | Tail(3);
  }
  return Lead;
}

// A component value constant of the json: {"type": ..., "value": ...}.
ComponentValVariant parseComponentValue(const simdjson::dom::element &Elem) {
  const std::string_view Type = Elem["type"];
  const simdjson::dom::element Value = Elem["value"];
  auto Text = [&Value]() {
    const std::string_view Str = Value;
    return std::string(Str);
  };
  if (Type == "bool"sv) {
    const bool Flag = Value;
    return ComponentValVariant{Flag};
  }
  if (Type == "string"sv) {
    return ComponentValVariant{Text()};
  }
  if (Type == "char"sv) {
    return ComponentValVariant{decodeCodepoint(Text())};
  }
  if (Type == "u8"sv) {
    return ComponentValVariant{static_cast<uint8_t>(std::stoul(Text()))};
  }
  if (Type == "s8"sv) {
    return ComponentValVariant{static_cast<int8_t>(std::stol(Text()))};
  }
  if (Type == "u16"sv) {
    return ComponentValVariant{static_cast<uint16_t>(std::stoul(Text()))};
  }
  if (Type == "s16"sv) {
    return ComponentValVariant{static_cast<int16_t>(std::stol(Text()))};
  }
  if (Type == "u32"sv) {
    return ComponentValVariant{static_cast<uint32_t>(std::stoul(Text()))};
  }
  if (Type == "s32"sv) {
    return ComponentValVariant{static_cast<int32_t>(std::stol(Text()))};
  }
  if (Type == "u64"sv) {
    return ComponentValVariant{static_cast<uint64_t>(std::stoull(Text()))};
  }
  if (Type == "s64"sv) {
    return ComponentValVariant{static_cast<int64_t>(std::stoll(Text()))};
  }
  // A float constant is its bit pattern.
  if (Type == "f32"sv) {
    const auto Bits = static_cast<uint32_t>(std::stoul(Text()));
    float Val;
    std::memcpy(&Val, &Bits, sizeof(Val));
    return ComponentValVariant{Val};
  }
  if (Type == "f64"sv) {
    const auto Bits = static_cast<uint64_t>(std::stoull(Text()));
    double Val;
    std::memcpy(&Val, &Bits, sizeof(Val));
    return ComponentValVariant{Val};
  }
  if (Type == "list"sv) {
    ListVal List;
    for (const simdjson::dom::element Item : simdjson::dom::array(Value)) {
      List.Elements.push_back(parseComponentValue(Item));
    }
    return makeComponentVal(std::move(List));
  }
  if (Type == "tuple"sv) {
    TupleVal Tuple;
    for (const simdjson::dom::element Item : simdjson::dom::array(Value)) {
      Tuple.Values.push_back(parseComponentValue(Item));
    }
    return makeComponentVal(std::move(Tuple));
  }
  // {"value": [["field", {...}], ...]}
  if (Type == "record"sv) {
    RecordVal Record;
    for (const simdjson::dom::array Field : simdjson::dom::array(Value)) {
      const std::string_view Name = Field.at(0);
      Record.Fields.emplace_back(std::string(Name),
                                 parseComponentValue(Field.at(1)));
    }
    return makeComponentVal(std::move(Record));
  }
  // {"value": {"case": "label", "payload": {...}}}, the payload optional.
  if (Type == "variant"sv) {
    VariantVal Variant;
    const std::string_view Label = Value["case"];
    Variant.Label = std::string(Label);
    simdjson::dom::element Payload;
    if (!Value["payload"].get(Payload) && !Payload.is_null()) {
      Variant.Payload = parseComponentValue(Payload);
    }
    return makeComponentVal(std::move(Variant));
  }
  if (Type == "enum"sv) {
    return makeComponentVal(EnumVal{0, Text()});
  }
  // {"value": ["set-label", ...]}
  if (Type == "flags"sv) {
    FlagsVal Flags;
    for (const std::string_view Label : simdjson::dom::array(Value)) {
      Flags.SetLabels.emplace_back(Label);
    }
    return makeComponentVal(std::move(Flags));
  }
  // {"value": {...}} for some, {"value": null} for none.
  if (Type == "option"sv) {
    OptionVal Option;
    if (!Value.is_null()) {
      Option.Value = parseComponentValue(Value);
    }
    return makeComponentVal(std::move(Option));
  }
  // {"value": {"Ok": {...}}} or {"value": {"Err": {...}}}, the payload
  // optional.
  if (Type == "result"sv) {
    ResultVal Result;
    simdjson::dom::element Payload;
    Result.IsOk = Value["Err"].get(Payload) != simdjson::error_code::SUCCESS;
    if (!Result.IsOk || !Value["Ok"].get(Payload)) {
      if (!Payload.is_null()) {
        Result.Payload = parseComponentValue(Payload);
      }
    }
    return makeComponentVal(std::move(Result));
  }
  ADD_FAILURE() << "unsupported component value type " << Type;
  return ComponentValVariant{};
}

// Helper function to parse component values from JSON, for parameters and
// for expected results alike.
std::vector<ComponentValVariant>
parseComponentValueList(const simdjson::dom::array &Args) {
  std::vector<ComponentValVariant> Result;
  Result.reserve(Args.size());
  for (const simdjson::dom::element Arg : Args) {
    Result.push_back(parseComponentValue(Arg));
  }
  return Result;
}

// Helper function to parse parameters from JSON to a vector of values.
std::pair<std::vector<WasmEdge::ValVariant>, std::vector<WasmEdge::ValType>>
parseValueList(const simdjson::dom::array &Args) {
  std::vector<WasmEdge::ValVariant> Result;
  std::vector<WasmEdge::ValType> ResultTypes;
  Result.reserve(Args.size());
  ResultTypes.reserve(Args.size());
  for (const simdjson::dom::object Element : Args) {
    std::string_view Type = Element["type"];
    simdjson::dom::element Value = Element["value"];
    if (Value.type() == simdjson::dom::element_type::ARRAY) {
      simdjson::dom::array ValueNodeArray = Value;
      WasmEdge::uint128_t V128;
      std::string_view LaneType = Element["lane_type"];
      if (LaneType == "i64"sv || LaneType == "f64"sv) {
        parseSIMDLanes<uint64_t>(V128, ValueNodeArray);
      } else if (LaneType == "i32"sv || LaneType == "f32"sv) {
        parseSIMDLanes<uint32_t>(V128, ValueNodeArray);
      } else if (LaneType == "i16"sv) {
        parseSIMDLanes<uint16_t>(V128, ValueNodeArray);
      } else if (LaneType == "i8"sv) {
        parseSIMDLanes<uint8_t>(V128, ValueNodeArray);
      } else {
        assumingUnreachable();
      }
      Result.emplace_back(V128);
      ResultTypes.emplace_back(WasmEdge::TypeCode::V128);
    } else if (Value.type() == simdjson::dom::element_type::STRING) {
      std::string_view ValueStr = Value;
      if (Type == "externref"sv || Type == "anyref"sv) {
        WasmEdge::TypeCode Code = Type == "externref"sv
                                      ? WasmEdge::TypeCode::ExternRef
                                      : WasmEdge::TypeCode::AnyRef;
        if (Value == "null"sv) {
          Result.emplace_back(WasmEdge::RefVariant(Code));
        } else {
          // ExternRef and AnyRef are non-opaque references. Add 0x1 uint32_t
          // prefix in this case to present non-null.
          Result.emplace_back(WasmEdge::RefVariant(
              Code, reinterpret_cast<void *>(std::stoul(std::string(ValueStr)) +
                                             0x100000000ULL)));
        }
        ResultTypes.emplace_back(Code);
      } else if (Type == "funcref"sv) {
        if (Value == "null"sv) {
          Result.emplace_back(
              WasmEdge::RefVariant(WasmEdge::TypeCode::FuncRef));
        } else {
          // Input values of opaque references are not supported for testing.
          assumingUnreachable();
        }
        ResultTypes.emplace_back(WasmEdge::TypeCode::FuncRef);
      } else if (Type == "i32"sv) {
        Result.emplace_back(
            static_cast<uint32_t>(std::stoul(std::string(ValueStr))));
        ResultTypes.emplace_back(WasmEdge::TypeCode::I32);
      } else if (Type == "f32"sv) {
        Result.emplace_back(
            static_cast<uint32_t>(std::stoul(std::string(ValueStr))));
        ResultTypes.emplace_back(WasmEdge::TypeCode::F32);
      } else if (Type == "i64"sv) {
        Result.emplace_back(
            static_cast<uint64_t>(std::stoull(std::string(ValueStr))));
        ResultTypes.emplace_back(WasmEdge::TypeCode::I64);
      } else if (Type == "f64"sv) {
        Result.emplace_back(
            static_cast<uint64_t>(std::stoull(std::string(ValueStr))));
        ResultTypes.emplace_back(WasmEdge::TypeCode::F64);
      } else {
        assumingUnreachable();
      }
    } else {
      assumingUnreachable();
    }
  }
  return {Result, ResultTypes};
}

// Helper function to parse parameters from JSON to a vector of string pairs.
std::vector<std::pair<std::string, std::string>>
parseExpectedList(const simdjson::dom::array &Args) {
  std::vector<std::pair<std::string, std::string>> Result;
  Result.reserve(Args.size());
  for (const simdjson::dom::object Element : Args) {
    std::string_view Type = Element["type"];
    simdjson::dom::element Value;
    auto NoValue = Element["value"].get(Value);
    if (NoValue) {
      // Only marked the result type, not check the opaque result reference
      // value.
      Result.emplace_back(std::string(Type), "");
    } else {
      if (Value.type() == simdjson::dom::element_type::ARRAY) {
        simdjson::dom::array ValueNodeArray = Value;
        std::string StrValue;
        std::string_view LaneType = Element["lane_type"];
        for (std::string_view X : ValueNodeArray) {
          StrValue += std::string(X);
          StrValue += ' ';
        }
        StrValue.pop_back();
        Result.emplace_back(std::string(Type) + std::string(LaneType),
                            std::move(StrValue));
      } else if (Value.type() == simdjson::dom::element_type::STRING) {
        std::string_view ValueStr = Value;
        Result.emplace_back(std::string(Type), std::string(ValueStr));
      } else {
        assumingUnreachable();
      }
    }
  }
  return Result;
}

std::vector<std::vector<std::pair<std::string, std::string>>>
parseEithersList(const simdjson::dom::array &Args) {
  std::vector<std::vector<std::pair<std::string, std::string>>> Result;
  Result.reserve(Args.size());
  for (auto &Maybe : parseExpectedList(Args)) {
    Result.emplace_back(
        std::vector<std::pair<std::string, std::string>>{Maybe});
  }
  return Result;
}

struct TestsuiteProposal {
  TestsuiteProposal(
      std::string_view P,
      const WasmEdge::Standard Std = WasmEdge::Standard::WASM_3,
      const std::vector<WasmEdge::Proposal> &EnableProps = {},
      const std::vector<WasmEdge::Proposal> &DisableProps = {},
      WasmEdge::SpecTest::TestMode M = WasmEdge::SpecTest::TestMode::All)
      : Path(P), Mode(M) {
    Conf.setWASMStandard(Std);
    for (const auto &Prop : EnableProps) {
      Conf.addProposal(Prop);
    }
    for (const auto &Prop : DisableProps) {
      Conf.removeProposal(Prop);
    }
  }

  std::string_view Path;
  WasmEdge::Configure Conf;
  WasmEdge::SpecTest::TestMode Mode = WasmEdge::SpecTest::TestMode::All;
};

static const TestsuiteProposal TestsuiteProposals[] = {
    // | Folder | WASM_Base | Additional_set | Removal_set | Mode |
    // ------------------------------------------------------------
    // Folder: the directory name of tests.
    // WASM_Base: the WASM standard base (1.0, 2.0, 3.0). Default: WASM 3.0
    // Additional_set: additional proposals to turn on. Default: {}
    // Removal_set: additional proposals to turn off. Default: {}
    // Mode: test execution modes (interpreter, AOT, JIT). Default: all
    {"wasm-1.0"sv, WasmEdge::Standard::WASM_1},
    {"wasm-2.0"sv, WasmEdge::Standard::WASM_2},
    {"wasm-3.0"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-bulk-memory"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-exceptions"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-gc"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-memory64"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-multi-memory"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-relaxed-simd"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-simd"sv, WasmEdge::Standard::WASM_3},
    {"threads"sv, WasmEdge::Standard::WASM_2, {Proposal::Threads}},
    // Currently, the component model supports only interpreter mode.
    {"component-model-binary"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::Component, Proposal::Threads},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"component-model-validation"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::Component, Proposal::Threads},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"component-model-resources"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::Component, Proposal::Threads},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"component-model-linking"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::Component, Proposal::Threads},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"component-model-async"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::Component, Proposal::Threads},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"component-model-values"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::Component, Proposal::Threads},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
};

bool isComponentProposal(std::string_view Path) {
  return Path.substr(0, 15) == "component-model"sv;
}

} // namespace

namespace WasmEdge {

std::vector<std::string> SpecTest::enumerate(const SpecTest::TestMode Mode,
                                             bool IncludeComponent) const {
  std::vector<std::string> Cases;
  for (const auto &Proposal : TestsuiteProposals) {
    if (static_cast<uint8_t>(Proposal.Mode) & static_cast<uint8_t>(Mode)) {
      if (!IncludeComponent && isComponentProposal(Proposal.Path)) {
        continue;
      }
      const std::filesystem::path ProposalRoot = TestsuiteRoot / Proposal.Path;
      for (const auto &Subdir :
           std::filesystem::directory_iterator(ProposalRoot)) {
        const auto SubdirPath = Subdir.path();
        const auto UnitName = u8string(SubdirPath.filename());
        const auto UnitJson = UnitName + ".json"s;
        if (std::filesystem::is_regular_file(SubdirPath / UnitJson)) {
          Cases.push_back(std::string(Proposal.Path) + ' ' + UnitName);
        }
      }
    }
  }
  std::sort(Cases.begin(), Cases.end());

  return Cases;
}

std::tuple<std::string_view, WasmEdge::Configure, std::string>
SpecTest::resolve(std::string_view Params) const {
  const auto Pos = Params.find_last_of(' ');
  const std::string_view ProposalPath = Params.substr(0, Pos);
  const auto &MatchedProposal = *std::find_if(
      std::begin(TestsuiteProposals), std::end(TestsuiteProposals),
      [&ProposalPath](const auto &Proposal) {
        return Proposal.Path == ProposalPath;
      });
  return std::tuple<std::string_view, WasmEdge::Configure, std::string>{
      MatchedProposal.Path, MatchedProposal.Conf, Params.substr(Pos + 1)};
}

bool SpecTest::compare(const std::pair<std::string, std::string> &Expected,
                       const std::pair<ValVariant, ValType> &Got) const {
  const auto &TypeStr = Expected.first;
  const auto &ValStr = Expected.second;

  auto IsRefMatch = [&ValStr](const WasmEdge::RefVariant &R) {
    if (ValStr == "null"sv) {
      // If explicitly expected a `null`, the reference must be null.
      return R.isNull();
    }
    if (ValStr == ""sv) {
      // Opaque expected reference. Always true.
      return true;
    }
    // Explicitly expected the reference value.
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(
               R.getPtr<void>())) == static_cast<uint32_t>(std::stoul(ValStr));
  };

  bool IsV128 = (std::string_view(TypeStr).substr(0, 4) == "v128"sv);
  if (!IsV128 && ValStr.substr(0, 4) == "nan:"sv) {
    // Handle NaN case
    // TODO: nan:canonical and nan:arithmetic
    if (TypeStr == "f32"sv) {
      if (Got.second.getCode() != TypeCode::F32) {
        return false;
      }
      return std::isnan(Got.first.get<float>());
    } else if (TypeStr == "f64"sv) {
      if (Got.second.getCode() != TypeCode::F64) {
        return false;
      }
      return std::isnan(Got.first.get<double>());
    }
  } else if (TypeStr == "ref"sv) {
    // "ref" fits all reference types.
    if (!Got.second.isRefType()) {
      return false;
    }
    return IsRefMatch(Got.first.get<RefVariant>());
  } else if (TypeStr == "anyref"sv) {
    // "anyref" fits all internal reference types.
    if (!Got.second.isRefType() || Got.second.isExternRefType()) {
      return false;
    }
    return IsRefMatch(Got.first.get<RefVariant>());
  } else if (TypeStr == "eqref"sv) {
    // "eqref" fits eqref, structref, arrayref, i31ref, and nullref.
    if (!Got.second.isRefType()) {
      return false;
    }
    switch (Got.second.getHeapTypeCode()) {
    case TypeCode::EqRef:
    case TypeCode::I31Ref:
    case TypeCode::StructRef:
    case TypeCode::ArrayRef:
    case TypeCode::NullRef:
      break;
    default:
      return false;
    }
    return IsRefMatch(Got.first.get<RefVariant>());
  } else if (TypeStr == "structref"sv) {
    // "structref" structref and nullref.
    if (!Got.second.isRefType()) {
      return false;
    }
    switch (Got.second.getHeapTypeCode()) {
    case TypeCode::StructRef:
    case TypeCode::NullRef:
      break;
    default:
      return false;
    }
    return IsRefMatch(Got.first.get<RefVariant>());
  } else if (TypeStr == "arrayref"sv) {
    // "arrayref" arrayref and nullref.
    if (!Got.second.isRefType()) {
      return false;
    }
    switch (Got.second.getHeapTypeCode()) {
    case TypeCode::ArrayRef:
    case TypeCode::NullRef:
      break;
    default:
      return false;
    }
    return IsRefMatch(Got.first.get<RefVariant>());
  } else if (TypeStr == "i31ref"sv) {
    // "i31ref" i31ref and nullref.
    if (!Got.second.isRefType()) {
      return false;
    }
    switch (Got.second.getHeapTypeCode()) {
    case TypeCode::I31Ref:
    case TypeCode::NullRef:
      break;
    default:
      return false;
    }
    return IsRefMatch(Got.first.get<RefVariant>());
  } else if (TypeStr == "nullref"sv) {
    if (!Got.second.isRefType() ||
        Got.second.getHeapTypeCode() != TypeCode::NullRef) {
      return false;
    }
    return IsRefMatch(Got.first.get<RefVariant>());
  } else if (TypeStr == "funcref"sv) {
    // "funcref" fits funcref and nullfuncref.
    if (!Got.second.isFuncRefType()) {
      return false;
    }
    return IsRefMatch(Got.first.get<RefVariant>());
  } else if (TypeStr == "nullfuncref"sv) {
    if (!Got.second.isRefType() ||
        Got.second.getHeapTypeCode() != TypeCode::NullFuncRef) {
      return false;
    }
    return IsRefMatch(Got.first.get<RefVariant>());
  } else if (TypeStr == "externref"sv) {
    // "externref" fits externref and nullexternref.
    if (!Got.second.isExternRefType()) {
      return false;
    }
    return IsRefMatch(Got.first.get<RefVariant>());
  } else if (TypeStr == "nullexternref"sv) {
    if (!Got.second.isRefType() ||
        Got.second.getHeapTypeCode() != TypeCode::NullExternRef) {
      return false;
    }
    return IsRefMatch(Got.first.get<RefVariant>());
  } else if (TypeStr == "exnref"sv) {
    // "exnref" fits exnref and nullexnref.
    if (!Got.second.isRefType() ||
        (Got.second.getHeapTypeCode() != TypeCode::ExnRef &&
         Got.second.getHeapTypeCode() != TypeCode::NullExnRef)) {
      return false;
    }
    return IsRefMatch(Got.first.get<RefVariant>());
  } else if (TypeStr == "nullexnref"sv) {
    if (!Got.second.isRefType() ||
        Got.second.getHeapTypeCode() != TypeCode::NullExnRef) {
      return false;
    }
    return IsRefMatch(Got.first.get<RefVariant>());
  } else if (TypeStr == "i32"sv) {
    if (Got.second.getCode() != TypeCode::I32) {
      return false;
    }
    return Got.first.get<uint32_t>() == uint32_t(std::stoul(ValStr));
  } else if (TypeStr == "f32"sv) {
    if (Got.second.getCode() != TypeCode::F32) {
      return false;
    }
    // Compare the 32-bit pattern
    return Got.first.get<uint32_t>() == uint32_t(std::stoul(ValStr));
  } else if (TypeStr == "i64"sv) {
    if (Got.second.getCode() != TypeCode::I64) {
      return false;
    }
    return Got.first.get<uint64_t>() == uint64_t(std::stoull(ValStr));
  } else if (TypeStr == "f64"sv) {
    if (Got.second.getCode() != TypeCode::F64) {
      return false;
    }
    // Compare the 64-bit pattern
    return Got.first.get<uint64_t>() == uint64_t(std::stoull(ValStr));
  } else if (IsV128) {
    std::vector<std::string_view> Parts;
    std::string_view Ev = ValStr;
    if (Got.second.getCode() != TypeCode::V128) {
      return false;
    }
    for (std::string::size_type Begin = 0, End = Ev.find(' ');
         Begin != std::string::npos;
         Begin = 1 + End, End = Ev.find(' ', Begin)) {
      Parts.push_back(Ev.substr(Begin, End - Begin));
      if (End == std::string::npos) {
        break;
      }
    }
    std::string_view LaneType = std::string_view(TypeStr).substr(4);
    if (LaneType == "f32") {
      float VF[4];
      uint32_t VI[4];
      std::memcpy(VF, &Got.first.get<uint128_t>(), 16);
      std::memcpy(VI, &Got.first.get<uint128_t>(), 16);
      if constexpr (Endian::native == Endian::big) {
        std::reverse(VI, VI + 4);
        std::reverse(VF, VF + 4);
      }
      for (size_t I = 0; I < 4; ++I) {
        if (Parts[I].substr(0, 4) == "nan:"sv) {
          if (!std::isnan(VF[I])) {
            return false;
          }
        } else {
          const uint32_t V1 = VI[I];
          const uint32_t V2 =
              static_cast<uint32_t>(std::stoul(std::string(Parts[I])));
          if (V1 != V2) {
            return false;
          }
        }
      }
    } else if (LaneType == "f64") {
      double VF[2];
      uint64_t VI[2];
      std::memcpy(VF, &Got.first.get<uint128_t>(), 16);
      std::memcpy(VI, &Got.first.get<uint128_t>(), 16);
      if constexpr (Endian::native == Endian::big) {
        std::reverse(VI, VI + 2);
        std::reverse(VF, VF + 2);
      }
      for (size_t I = 0; I < 2; ++I) {
        if (Parts[I].substr(0, 4) == "nan:"sv) {
          if (!std::isnan(VF[I])) {
            return false;
          }
        } else {
          const uint64_t V1 = VI[I];
          const uint64_t V2 = std::stoull(std::string(Parts[I]));
          if (V1 != V2) {
            return false;
          }
        }
      }
    } else if (LaneType == "i8") {
      uint8_t V[16];
      std::memcpy(V, &Got.first.get<uint128_t>(), 16);
      if constexpr (Endian::native == Endian::big) {
        std::reverse(V, V + 16);
      }
      for (size_t I = 0; I < 16; ++I) {
        const uint8_t V1 = V[I];
        const uint8_t V2 =
            static_cast<uint8_t>(std::stoul(std::string(Parts[I])));
        if (V1 != V2) {
          return false;
        }
      }
    } else if (LaneType == "i16") {
      uint16_t V[8];
      std::memcpy(V, &Got.first.get<uint128_t>(), 16);
      if constexpr (Endian::native == Endian::big) {
        std::reverse(V, V + 8);
      }
      for (size_t I = 0; I < 8; ++I) {
        const uint16_t V1 = V[I];
        const uint16_t V2 =
            static_cast<uint16_t>(std::stoul(std::string(Parts[I])));
        if (V1 != V2) {
          return false;
        }
      }
    } else if (LaneType == "i32") {
      uint32_t V[4];
      std::memcpy(V, &Got.first.get<uint128_t>(), 16);
      if constexpr (Endian::native == Endian::big) {
        std::reverse(V, V + 4);
      }
      for (size_t I = 0; I < 4; ++I) {
        const uint32_t V1 = V[I];
        const uint32_t V2 =
            static_cast<uint32_t>(std::stoul(std::string(Parts[I])));
        if (V1 != V2) {
          return false;
        }
      }
    } else if (LaneType == "i64") {
      uint64_t V[2];
      std::memcpy(V, &Got.first.get<uint128_t>(), 16);
      if constexpr (Endian::native == Endian::big) {
        std::reverse(V, V + 2);
      }
      for (size_t I = 0; I < 2; ++I) {
        const uint64_t V1 = V[I];
        const uint64_t V2 = std::stoull(std::string(Parts[I]));
        if (V1 != V2) {
          return false;
        }
      }
    } else {
      return false;
    }
    return true;
  }
  return false;
}

bool SpecTest::compares(
    const std::vector<std::pair<std::string, std::string>> &Expected,
    const std::vector<std::pair<ValVariant, ValType>> &Got) const {
  if (Expected.size() != Got.size()) {
    return false;
  }
  for (size_t I = 0; I < Expected.size(); ++I) {
    if (!compare(Expected[I], Got[I])) {
      return false;
    }
  }
  return true;
}

bool SpecTest::compare(const ComponentValVariant &Expected,
                       const ComponentValVariant &Got) const {
  if (Expected.index() != Got.index()) {
    return false;
  }
  if (!std::holds_alternative<std::shared_ptr<ValComp>>(Expected)) {
    return Expected == Got;
  }
  const auto &CompE = std::get<std::shared_ptr<ValComp>>(Expected);
  const auto &CompG = std::get<std::shared_ptr<ValComp>>(Got);
  if (!CompE || !CompG || CompE->V.index() != CompG->V.index()) {
    return false;
  }
  auto SameOptional = [this](const std::optional<ComponentValVariant> &E,
                             const std::optional<ComponentValVariant> &G) {
    return E.has_value() == G.has_value() &&
           (!E.has_value() || compare(*E, *G));
  };
  // A case is named by label in the json and by index once lifted.
  auto SameCase = [](uint32_t CaseE, std::string_view LabelE, uint32_t CaseG,
                     std::string_view LabelG) {
    return !LabelE.empty() && !LabelG.empty() ? LabelE == LabelG
                                              : CaseE == CaseG;
  };
  if (const auto *List = std::get_if<ListVal>(&CompE->V)) {
    return compares(List->Elements, std::get<ListVal>(CompG->V).Elements);
  }
  if (const auto *Tuple = std::get_if<TupleVal>(&CompE->V)) {
    return compares(Tuple->Values, std::get<TupleVal>(CompG->V).Values);
  }
  if (const auto *Record = std::get_if<RecordVal>(&CompE->V)) {
    const auto &Other = std::get<RecordVal>(CompG->V);
    if (Record->Fields.size() != Other.Fields.size()) {
      return false;
    }
    for (size_t I = 0; I < Record->Fields.size(); ++I) {
      if (Record->Fields[I].first != Other.Fields[I].first ||
          !compare(Record->Fields[I].second, Other.Fields[I].second)) {
        return false;
      }
    }
    return true;
  }
  if (const auto *Variant = std::get_if<VariantVal>(&CompE->V)) {
    const auto &Other = std::get<VariantVal>(CompG->V);
    return SameCase(Variant->Case, Variant->Label, Other.Case, Other.Label) &&
           SameOptional(Variant->Payload, Other.Payload);
  }
  if (const auto *Enum = std::get_if<EnumVal>(&CompE->V)) {
    const auto &Other = std::get<EnumVal>(CompG->V);
    return SameCase(Enum->Case, Enum->Label, Other.Case, Other.Label);
  }
  if (const auto *Option = std::get_if<OptionVal>(&CompE->V)) {
    return SameOptional(Option->Value, std::get<OptionVal>(CompG->V).Value);
  }
  if (const auto *Result = std::get_if<ResultVal>(&CompE->V)) {
    const auto &Other = std::get<ResultVal>(CompG->V);
    return Result->IsOk == Other.IsOk &&
           SameOptional(Result->Payload, Other.Payload);
  }
  if (const auto *Flags = std::get_if<FlagsVal>(&CompE->V)) {
    // A lifted flags value carries the bits and the labels: compare labels.
    auto Sorted = [](const FlagsVal &Val) {
      std::vector<std::string> Labels = Val.SetLabels;
      std::sort(Labels.begin(), Labels.end());
      return Labels;
    };
    return Sorted(*Flags) == Sorted(std::get<FlagsVal>(CompG->V));
  }
  return false;
}

bool SpecTest::compares(const std::vector<ComponentValVariant> &Expected,
                        const std::vector<ComponentValVariant> &Got) const {
  if (Expected.size() != Got.size()) {
    return false;
  }
  for (size_t I = 0; I < Expected.size(); ++I) {
    if (!compare(Expected[I], Got[I])) {
      return false;
    }
  }
  return true;
}

bool SpecTest::compares(
    const std::vector<ComponentValVariant> &Expected,
    const std::vector<std::pair<ComponentValVariant, ComponentValType>> &Got)
    const {
  if (Expected.size() != Got.size()) {
    return false;
  }
  for (size_t I = 0; I < Expected.size(); ++I) {
    if (!compare(Expected[I], Got[I].first)) {
      return false;
    }
  }
  return true;
}

bool SpecTest::stringContains(std::string_view Expected,
                              std::string_view Got) const {
  if (Expected.rfind(Got, 0) != 0) {
    spdlog::error("   ##### expected text : {}"sv, Expected);
    spdlog::error("   ######## error text : {}"sv, Got);
    return false;
  }
  return true;
}

void SpecTest::run(std::string_view Proposal, std::string_view UnitName) {
  spdlog::info("{} {}"sv, Proposal, UnitName);
  auto TestFileName =
      (TestsuiteRoot / Proposal / UnitName / (std::string(UnitName) + ".json"s))
          .string();

  simdjson::dom::parser Parser;
  simdjson::dom::element Doc = Parser.load(TestFileName);

  simdjson::dom::array CmdArray;
  if (!Doc["commands"].get(CmdArray)) {
    // Create root context (no parent, no shared modules).
    auto Ctx = onInit(nullptr, {});
    processCommands(Ctx, Proposal, UnitName, &CmdArray);
    onFini(Ctx);
  }
}

void SpecTest::processCommands(ContextHandle Ctx, std::string_view Proposal,
                               std::string_view UnitName, void *CmdArrayPtr) {
  simdjson::dom::array CmdArray =
      *static_cast<simdjson::dom::array *>(CmdArrayPtr);
  const bool IsComponent = isComponentProposal(Proposal);

  std::map<std::string, std::string> Alias;
  std::map<std::string, SpecTest::WasmUnit> ASTMap;
  std::string LastModName;
  std::map<std::string, std::thread> ThreadMap;

  // Helper function to get module name.
  auto GetModuleName = [&](const simdjson::dom::object &Action) -> std::string {
    std::string_view ModName;
    if (!Action["module"].get(ModName)) {
      if (auto It = Alias.find(std::string(ModName)); It != Alias.end()) {
        // If module name is aliased, use the aliased name.
        return It->second;
      }
      return std::string(ModName);
    }
    return LastModName;
  };

  // Helper function to invoke a component function with the json arguments.
  auto CompInvoke = [&](const simdjson::dom::object &Action) {
    const std::string_view Field = Action["field"];
    return onCompInvoke(Ctx, GetModuleName(Action), std::string(Field),
                        parseComponentValueList(Action["args"]));
  };

  // Helper function to invoke a core function with the json arguments.
  auto CoreInvoke = [&](const simdjson::dom::object &Action) {
    const std::string_view Field = Action["field"];
    // Named modules are registered in the store manager; anonymous modules
    // are instantiated in the VM.
    const auto Params = parseValueList(Action["args"]);
    return onInvoke(Ctx, GetModuleName(Action), std::string(Field),
                    Params.first, Params.second);
  };

  // Helper function to check the result of invocation.
  auto Invoke = [&](const simdjson::dom::object &Action,
                    const simdjson::dom::array &Expected, uint64_t LineNumber) {
    if (IsComponent) {
      if (auto Res = CompInvoke(Action)) {
        EXPECT_TRUE(compares(parseComponentValueList(Expected), *Res));
      } else {
        EXPECT_NE(LineNumber, LineNumber);
      }
    } else {
      if (auto Res = CoreInvoke(Action)) {
        EXPECT_TRUE(compares(parseExpectedList(Expected), *Res));
      } else {
        EXPECT_NE(LineNumber, LineNumber);
      }
    }
  };

  // Helper function to run an invocation for its side effects.
  auto Run = [&](const simdjson::dom::object &Action, uint64_t LineNumber) {
    if (IsComponent) {
      if (!CompInvoke(Action)) {
        EXPECT_NE(LineNumber, LineNumber);
      }
    } else {
      if (!CoreInvoke(Action)) {
        EXPECT_NE(LineNumber, LineNumber);
      }
    }
  };

  // Helper function to check one of the matched results of invocation.
  auto InvokeEither = [&](const simdjson::dom::object &Action,
                          const simdjson::dom::array &Eithers,
                          uint64_t LineNumber) {
    // The component suites have no `either` results.
    if (IsComponent) {
      EXPECT_NE(LineNumber, LineNumber);
    } else {
      if (auto Res = CoreInvoke(Action)) {
        const auto Returns = parseEithersList(Eithers);
        const bool Matched =
            std::any_of(Returns.begin(), Returns.end(), [&](const auto &Maybe) {
              return compares(Maybe, *Res);
            });
        EXPECT_TRUE(Matched) << "This is One of available returns.";
      } else {
        EXPECT_NE(LineNumber, LineNumber);
      }
    }
  };

  // Helper function to get values.
  auto Get = [&](const simdjson::dom::object &Action,
                 const simdjson::dom::array &Expected, uint64_t LineNumber) {
    // A component exports no global.
    if (IsComponent) {
      EXPECT_NE(LineNumber, LineNumber);
    } else {
      const std::string_view Field = Action["field"];
      if (auto Res = onGet(Ctx, GetModuleName(Action), std::string(Field))) {
        EXPECT_TRUE(compare(parseExpectedList(Expected)[0], *Res));
      } else {
        EXPECT_NE(LineNumber, LineNumber);
      }
    }
  };

  // Helper function to check trap on loading.
  auto TrapLoad = [&](const std::string &FileName, const std::string &Text) {
    auto Res = IsComponent ? onCompLoad(Ctx, FileName) : onLoad(Ctx, FileName);
    if (Res) {
      EXPECT_TRUE(false);
    } else {
      EXPECT_TRUE(Res.error().getErrCodePhase() == WasmPhase::Loading);
      EXPECT_TRUE(stringContains(Text, ErrCodeStr[Res.error().getEnum()]));
    }
  };

  // Helper function to check trap on validation.
  auto TrapValidate = [&](const std::string &FileName,
                          const std::string &Text) {
    auto Res =
        IsComponent ? onCompValidate(Ctx, FileName) : onValidate(Ctx, FileName);
    if (Res) {
      EXPECT_TRUE(false);
    } else {
      EXPECT_TRUE(Res.error().getErrCodePhase() == WasmPhase::Validation);
      EXPECT_TRUE(stringContains(Text, ErrCodeStr[Res.error().getEnum()]));
    }
  };

  // Helper function to check trap on instantiation.
  auto TrapInstantiate = [&](const std::string &FileName,
                             const std::string &Text) {
    auto Res = IsComponent ? onCompInstantiate(Ctx, FileName)
                           : onInstantiate(Ctx, FileName);
    if (Res) {
      EXPECT_TRUE(false);
    } else {
      EXPECT_TRUE(Res.error().getErrCodePhase() == WasmPhase::Instantiation ||
                  Res.error().getErrCodePhase() == WasmPhase::Execution);
      EXPECT_TRUE(stringContains(Text, ErrCodeStr[Res.error().getEnum()]));
    }
  };

  // Helper function to check trap on invocation.
  auto TrapInvoke = [&](const simdjson::dom::object &Action,
                        const std::string &Text, uint64_t LineNumber) {
    if (IsComponent) {
      if (auto Res = CompInvoke(Action)) {
        EXPECT_NE(LineNumber, LineNumber);
      } else {
        EXPECT_TRUE(stringContains(Text, ErrCodeStr[Res.error().getEnum()]));
      }
    } else {
      if (auto Res = CoreInvoke(Action)) {
        EXPECT_NE(LineNumber, LineNumber);
      } else {
        EXPECT_TRUE(Res.error().getErrCodePhase() == WasmPhase::Execution);
        EXPECT_TRUE(stringContains(Text, ErrCodeStr[Res.error().getEnum()]));
      }
    }
  };

  // Helper function to check exception on invocation.
  auto ExceptionInvoke = [&](const simdjson::dom::object &Action,
                             uint64_t LineNumber) {
    // The component suites have no `assert_exception`.
    if (IsComponent) {
      EXPECT_NE(LineNumber, LineNumber);
    } else {
      if (auto Res = CoreInvoke(Action)) {
        EXPECT_NE(LineNumber, LineNumber);
      } else {
        EXPECT_EQ(Res.error(), ErrCode::Value::UncaughtException);
      }
    }
  };

  // Preprocessing register command.
  resolveRegister(Alias, CmdArray);

  // Command processing. Return true for expected result.
  auto RunCommand = [&](const simdjson::dom::object &Cmd) {
    std::string_view TypeField;

    if (!Cmd["type"].get(TypeField)) {
      switch (resolveCommand(TypeField)) {
      case SpecTest::CommandID::Module: {
        std::string_view ModType;
        if (!Cmd["module_type"].get(ModType)) {
          if (ModType != "binary"sv) {
            // TODO: Wat is not supported in WasmEdge yet.
            return;
          }
        }
        std::string_view FileName = Cmd["filename"];
        const auto FilePath =
            u8string(TestsuiteRoot / Proposal / UnitName / FileName);
        const uint64_t LineNumber = Cmd["line"];
        std::string LineStr = std::to_string(LineNumber);
        std::string_view TempName;
        if (!Cmd["name"].get(TempName)) {
          // Module has name. Register module with module name.
          if (auto It = Alias.find(std::string(TempName)); It != Alias.end()) {
            LastModName = It->second;
          } else {
            LastModName = TempName;
          }
        } else if (auto It = Alias.find(LineStr); It != Alias.end()) {
          LastModName = It->second;
        } else {
          // Instantiate the anonymous module.
          LastModName.clear();
        }
        auto Res = IsComponent ? onCompModule(Ctx, LastModName, FilePath)
                               : onModule(Ctx, LastModName, FilePath);
        if (Res) {
          EXPECT_TRUE(true);
        } else {
          EXPECT_NE(LineNumber, LineNumber);
        }
        return;
      }
      case CommandID::ModuleDefinition: {
        std::string_view ASTName;
        std::string_view FileName = Cmd["filename"];
        const auto FilePath =
            u8string(TestsuiteRoot / Proposal / UnitName / FileName);
        const uint64_t LineNumber = Cmd["line"];
        if (auto Res = onModuleDefine(Ctx, std::string(FilePath)); Res) {
          if (!Cmd["name"].get(ASTName)) {
            ASTMap.emplace(std::string(ASTName), std::move(*Res));
          }
          EXPECT_TRUE(true);
        } else {
          EXPECT_NE(LineNumber, LineNumber);
        }
        return;
      }
      case CommandID::ModuleInstance: {
        // wast2json names the pair "name" and "definition", json-from-wast
        // "instance" and "module".
        std::string_view ModName;
        std::string_view ASTName;
        if (Cmd["instance"].get(ModName) != simdjson::error_code::SUCCESS) {
          ModName = Cmd["name"];
        }
        if (Cmd["module"].get(ASTName) != simdjson::error_code::SUCCESS) {
          ASTName = Cmd["definition"];
        }
        const uint64_t LineNumber = Cmd["line"];
        auto ASTDef = ASTMap.find(std::string(ASTName));
        if (ASTDef == ASTMap.end()) {
          EXPECT_NE(LineNumber, LineNumber);
          return;
        }
        if (auto It = Alias.find(std::string(ModName)); It != Alias.end()) {
          ModName = It->second;
        }
        LastModName = ModName;
        Expect<void> Res;
        if (IsComponent) {
          auto &ASTComp = *std::get<std::unique_ptr<AST::Component::Component>>(
              ASTDef->second);
          Res = onCompInstanceFromDef(Ctx, std::string(ModName), ASTComp);
        } else {
          auto &ASTMod =
              *std::get<std::unique_ptr<AST::Module>>(ASTDef->second);
          Res = onInstanceFromDef(Ctx, std::string(ModName), ASTMod);
        }
        if (Res) {
          EXPECT_TRUE(true);
        } else {
          EXPECT_NE(LineNumber, LineNumber);
        }
        return;
      }
      case CommandID::Action: {
        const simdjson::dom::object &Action = Cmd["action"];
        const uint64_t LineNumber = Cmd["line"];
        simdjson::dom::array Expected;
        if (Cmd["expected"].get(Expected) == simdjson::error_code::SUCCESS) {
          Invoke(Action, Expected, LineNumber);
        } else {
          Run(Action, LineNumber);
        }
        return;
      }
      case CommandID::Register: {
        // Preprocessed. Ignore this.
        return;
      }
      case CommandID::AssertReturn: {
        const uint64_t LineNumber = Cmd["line"];
        const simdjson::dom::object &Action = Cmd["action"];
        const std::string_view ActType = Action["type"];
        simdjson::dom::array Exp, Either;

        if (Cmd["expected"].get(Exp) == simdjson::error_code::SUCCESS) {
          if (ActType == "invoke"sv) {
            Invoke(Action, Exp, LineNumber);
            return;
          } else if (ActType == "get"sv) {
            Get(Action, Exp, LineNumber);
            return;
          }
        } else if (Cmd["either"].get(Either) == simdjson::error_code::SUCCESS) {
          if (ActType == "invoke"sv) {
            InvokeEither(Action, Either, LineNumber);
            return;
          }
        }

        EXPECT_TRUE(false);
        return;
      }
      case CommandID::AssertTrap: {
        const simdjson::dom::object &Action = Cmd["action"];
        const std::string_view Text = Cmd["text"];
        const uint64_t LineNumber = Cmd["line"];
        TrapInvoke(Action, std::string(Text), LineNumber);
        return;
      }
      case CommandID::AssertExhaustion: {
        // TODO: Add stack overflow mechanism.
        return;
      }
      case CommandID::AssertMalformed: {
        const std::string_view ModType = Cmd["module_type"];
        if (ModType != "binary"sv) {
          // TODO: Wat is not supported in WasmEdge yet.
          return;
        }
        const std::string_view Name = Cmd["filename"];
        const auto Filename =
            u8string(TestsuiteRoot / Proposal / UnitName / Name);
        const std::string_view Text = Cmd["text"];
        TrapLoad(Filename, std::string(Text));
        return;
      }
      case CommandID::AssertInvalid: {
        const std::string_view ModType = Cmd["module_type"];
        if (ModType != "binary"sv) {
          // TODO: Wat is not supported in WasmEdge yet.
          return;
        }
        const std::string_view Name = Cmd["filename"];
        const auto Filename =
            u8string(TestsuiteRoot / Proposal / UnitName / Name);
        const std::string_view Text = Cmd["text"];
        TrapValidate(Filename, std::string(Text));
        return;
      }
      case CommandID::AssertUnlinkable:
      case CommandID::AssertUninstantiable: {
        const std::string_view Name = Cmd["filename"];
        const auto Filename =
            u8string(TestsuiteRoot / Proposal / UnitName / Name);
        const std::string_view Text = Cmd["text"];
        TrapInstantiate(Filename, std::string(Text));
        return;
      }
      case CommandID::AssertException: {
        const simdjson::dom::object &Action = Cmd["action"];
        const std::string_view ActType = Action["type"];
        const uint64_t LineNumber = Cmd["line"];
        // TODO: Check the expected exception type.
        if (ActType == "invoke"sv) {
          ExceptionInvoke(Action, LineNumber);
          return;
        }
        EXPECT_TRUE(false);
        return;
      }
      case CommandID::Thread: {
        if (!onInit) {
          // Thread support not wired — skip.
          return;
        }
        std::string_view ThreadName = Cmd["name"];
        simdjson::dom::array ThreadCmds = Cmd["commands"];

        // Build shared module mapping: (parentStoreName, threadAliasName).
        // Pre-scan the thread's commands for register entries to determine
        // the alias names. The shared field tells us which modules to share,
        // and the register commands inside the thread tell us what names
        // to register them under.
        std::map<std::string, std::string> SharedRegisterMap;
        for (const simdjson::dom::object SubCmd : ThreadCmds) {
          std::string_view SubType;
          if (!SubCmd["type"].get(SubType) && SubType == "register"sv) {
            std::string_view RegName, RegAs;
            if (!SubCmd["name"].get(RegName) && !SubCmd["as"].get(RegAs)) {
              SharedRegisterMap.emplace(std::string(RegName),
                                        std::string(RegAs));
            }
          }
        }

        std::vector<std::pair<std::string, std::string>> SharedModules;
        simdjson::dom::array SharedArray;
        if (!Cmd["shared"].get(SharedArray)) {
          for (const simdjson::dom::object SharedEntry : SharedArray) {
            std::string_view ModRef = SharedEntry["module"];
            std::string OrigName(ModRef);
            // Resolve parent store name through alias.
            std::string ParentName = OrigName;
            if (auto It = Alias.find(OrigName); It != Alias.end()) {
              ParentName = It->second;
            }
            // Find alias name from thread's register commands.
            std::string AliasName = ParentName;
            if (auto It = SharedRegisterMap.find(OrigName);
                It != SharedRegisterMap.end()) {
              AliasName = It->second;
            }
            SharedModules.emplace_back(std::move(ParentName),
                                       std::move(AliasName));
          }
        }

        // Create child context with shared module mapping.
        auto ChildCtx = onInit(Ctx, SharedModules);

        // Spawn thread with child context.
        auto ThreadNameStr = std::string(ThreadName);
        ThreadMap.emplace(
            ThreadNameStr,
            std::thread([this, ChildCtx, P = std::string(Proposal),
                         U = std::string(UnitName), ThreadCmds]() {
              simdjson::dom::array Cmds = ThreadCmds;
              processCommands(ChildCtx, P, U, &Cmds);
              onFini(ChildCtx);
            }));
        return;
      }
      case CommandID::Wait: {
        std::string_view ThreadName = Cmd["thread"];
        auto It = ThreadMap.find(std::string(ThreadName));
        if (It != ThreadMap.end()) {
          if (It->second.joinable()) {
            It->second.join();
          }
          ThreadMap.erase(It);
        } else {
          const uint64_t LineNumber = Cmd["line"];
          EXPECT_NE(LineNumber, LineNumber)
              << "Wait for unknown thread: " << ThreadName;
        }
        return;
      }
      default:;
      }
    }
    // Unknown command.
    EXPECT_TRUE(false);
  };

  // Iterate commands.
  for (const simdjson::dom::object Cmd : CmdArray) {
    RunCommand(Cmd);
  }

  // Safety: join any threads not explicitly waited on.
  for (auto &[Name, Thread] : ThreadMap) {
    if (Thread.joinable()) {
      Thread.join();
    }
  }
}

} // namespace WasmEdge
