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
#include "common/errcode.h"
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
#include <thread>
#include <unordered_map>
#include <variant>

namespace WasmEdge {}

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

// Component-model value helpers: parse a json-from-wast ComponentConst into
// a ComponentValVariant, and compare invoke results against expectations.
namespace {

uint32_t decodeFirstCodepoint(std::string_view S) {
  if (S.empty()) {
    return 0;
  }
  const auto B0 = static_cast<uint8_t>(S[0]);
  if (B0 < 0x80) {
    return B0;
  }
  if ((B0 & 0xE0U) == 0xC0U && S.size() >= 2) {
    return ((B0 & 0x1FU) << 6) | (static_cast<uint8_t>(S[1]) & 0x3FU);
  }
  if ((B0 & 0xF0U) == 0xE0U && S.size() >= 3) {
    return ((B0 & 0x0FU) << 12) | ((static_cast<uint8_t>(S[1]) & 0x3FU) << 6) |
           (static_cast<uint8_t>(S[2]) & 0x3FU);
  }
  if ((B0 & 0xF8U) == 0xF0U && S.size() >= 4) {
    return ((B0 & 0x07U) << 18) | ((static_cast<uint8_t>(S[1]) & 0x3FU) << 12) |
           ((static_cast<uint8_t>(S[2]) & 0x3FU) << 6) |
           (static_cast<uint8_t>(S[3]) & 0x3FU);
  }
  return B0;
}

// NOLINTNEXTLINE(misc-no-recursion)
WasmEdge::ComponentValVariant
parseComponentValue(const simdjson::dom::element &Elem) {
  using namespace WasmEdge;
  const std::string_view Type = Elem["type"];
  auto Value = Elem["value"];
  auto AsStr = [&]() {
    std::string_view V = Value;
    return std::string(V);
  };
  if (Type == "bool"sv) {
    bool B = Value;
    return ComponentValVariant{B};
  }
  if (Type == "string"sv) {
    return ComponentValVariant{AsStr()};
  }
  if (Type == "char"sv) {
    return ComponentValVariant{decodeFirstCodepoint(AsStr())};
  }
  if (Type == "u8"sv) {
    return ComponentValVariant{uint8_t(std::stoul(AsStr()))};
  }
  if (Type == "s8"sv) {
    return ComponentValVariant{int8_t(std::stol(AsStr()))};
  }
  if (Type == "u16"sv) {
    return ComponentValVariant{uint16_t(std::stoul(AsStr()))};
  }
  if (Type == "s16"sv) {
    return ComponentValVariant{int16_t(std::stol(AsStr()))};
  }
  if (Type == "u32"sv) {
    return ComponentValVariant{uint32_t(std::stoul(AsStr()))};
  }
  if (Type == "s32"sv) {
    return ComponentValVariant{int32_t(std::stol(AsStr()))};
  }
  if (Type == "u64"sv) {
    return ComponentValVariant{uint64_t(std::stoull(AsStr()))};
  }
  if (Type == "s64"sv) {
    return ComponentValVariant{int64_t(std::stoll(AsStr()))};
  }
  if (Type == "f32"sv) {
    // The json carries the bit pattern.
    uint32_t Bits = uint32_t(std::stoul(AsStr()));
    float F;
    std::memcpy(&F, &Bits, sizeof(F));
    return ComponentValVariant{F};
  }
  if (Type == "f64"sv) {
    uint64_t Bits = uint64_t(std::stoull(AsStr()));
    double D;
    std::memcpy(&D, &Bits, sizeof(D));
    return ComponentValVariant{D};
  }
  if (Type == "list"sv || Type == "tuple"sv) {
    simdjson::dom::array Arr = Value;
    if (Type == "list"sv) {
      WasmEdge::ListVal L;
      for (auto E : Arr) {
        L.Elements.push_back(parseComponentValue(E));
      }
      return makeComponentVal(std::move(L));
    }
    WasmEdge::TupleVal T;
    for (auto E : Arr) {
      T.Values.push_back(parseComponentValue(E));
    }
    return makeComponentVal(std::move(T));
  }
  if (Type == "record"sv) {
    // {"value": [["field-name", {...}], ...]}
    WasmEdge::RecordVal R;
    simdjson::dom::array Fields = Value;
    for (auto F : Fields) {
      simdjson::dom::array Pair = F;
      std::string_view FieldName = Pair.at(0);
      R.Fields.emplace_back(std::string(FieldName),
                            parseComponentValue(Pair.at(1)));
    }
    return makeComponentVal(std::move(R));
  }
  if (Type == "variant"sv) {
    // {"value": {"case": "name", "payload": {...}?}}
    WasmEdge::VariantVal V;
    std::string_view CaseName = Value["case"];
    V.Case = 0;
    V.Label = std::string(CaseName);
    simdjson::dom::element Payload;
    if (!Value["payload"].get(Payload) && !Payload.is_null()) {
      V.Payload = parseComponentValue(Payload);
    }
    return makeComponentVal(std::move(V));
  }
  if (Type == "enum"sv) {
    // {"value": "label"}
    return makeComponentVal(WasmEdge::EnumVal{0, AsStr()});
  }
  if (Type == "flags"sv) {
    // {"value": ["set-label", ...]}
    WasmEdge::FlagsVal F;
    simdjson::dom::array Labels = Value;
    for (std::string_view L : Labels) {
      F.SetLabels.emplace_back(L);
    }
    return makeComponentVal(std::move(F));
  }
  if (Type == "option"sv) {
    // {"value": {...}} for some, {"value": null} for none.
    WasmEdge::OptionVal O;
    if (!Value.is_null()) {
      O.Value = parseComponentValue(Value.value());
    }
    return makeComponentVal(std::move(O));
  }
  if (Type == "result"sv) {
    // {"value": {"Ok": {...}?}} or {"value": {"Err": {...}?}}
    WasmEdge::ResultVal R;
    simdjson::dom::element Payload;
    if (!Value["Ok"].get(Payload)) {
      R.IsOk = true;
    } else if (!Value["Err"].get(Payload)) {
      R.IsOk = false;
    } else {
      R.IsOk = true;
      return makeComponentVal(std::move(R));
    }
    if (!Payload.is_null()) {
      R.Payload = parseComponentValue(Payload);
    }
    return makeComponentVal(std::move(R));
  }
  // Unsupported kinds compare as never-equal placeholders.
  return ComponentValVariant{std::string("<unsupported>")};
}

// NOLINTNEXTLINE(misc-no-recursion)
bool equalComponentValue(const WasmEdge::ComponentValVariant &A,
                         const WasmEdge::ComponentValVariant &B) {
  using namespace WasmEdge;
  if (A.index() != B.index()) {
    return false;
  }
  if (std::holds_alternative<std::shared_ptr<ValComp>>(A)) {
    const auto &CA = std::get<std::shared_ptr<ValComp>>(A);
    const auto &CB = std::get<std::shared_ptr<ValComp>>(B);
    if (!CA || !CB || CA->V.index() != CB->V.index()) {
      return false;
    }
    if (std::holds_alternative<ListVal>(CA->V)) {
      const auto &LA = std::get<ListVal>(CA->V);
      const auto &LB = std::get<ListVal>(CB->V);
      if (LA.Elements.size() != LB.Elements.size()) {
        return false;
      }
      for (size_t I = 0; I < LA.Elements.size(); ++I) {
        if (!equalComponentValue(LA.Elements[I], LB.Elements[I])) {
          return false;
        }
      }
      return true;
    }
    if (std::holds_alternative<TupleVal>(CA->V)) {
      const auto &TA = std::get<TupleVal>(CA->V);
      const auto &TB = std::get<TupleVal>(CB->V);
      if (TA.Values.size() != TB.Values.size()) {
        return false;
      }
      for (size_t I = 0; I < TA.Values.size(); ++I) {
        if (!equalComponentValue(TA.Values[I], TB.Values[I])) {
          return false;
        }
      }
      return true;
    }
    // Other aggregates are not produced by the current test suites.
    return false;
  }
  return A == B;
}

} // namespace

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
    // TODO: EXCEPTION - implement the AOT.
    {"wasm-3.0-exceptions"sv,
     WasmEdge::Standard::WASM_3,
     {},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"wasm-3.0-gc"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-memory64"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-multi-memory"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-relaxed-simd"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-simd"sv, WasmEdge::Standard::WASM_3},
    {"threads"sv, WasmEdge::Standard::WASM_2, {Proposal::Threads}},
    // Currently, the component model supports only interpreter mode.
    {"component-model-wasm-tools"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::Component, Proposal::Threads},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"component-model-wasmtime"sv,
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

constexpr std::string_view ComponentProposalPrefix = "component-model"sv;

bool isComponentProposal(std::string_view Path) noexcept {
  return Path.compare(0, ComponentProposalPrefix.size(),
                      ComponentProposalPrefix) == 0;
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
        const auto UnitName = SubdirPath.filename().u8string();
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

bool SpecTest::stringContains(std::string_view Expected,
                              std::string_view Got) const {
  // Reference-style matching: accept when either message contains the other
  // (wasm-tools emits multi-line diagnostics that the assertions quote
  // partially).
  if (Expected.find(Got) == std::string_view::npos &&
      Got.find(Expected) == std::string_view::npos) {
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

  // Helper function to check the result of invocation.
  auto Invoke = [&](const simdjson::dom::object &Action,
                    const simdjson::dom::array &Expected, uint64_t LineNumber) {
    if (IsComponent) {
      const auto CompModName = GetModuleName(Action);
      const std::string_view CompField = Action["field"];
      simdjson::dom::array CompArgs = Action["args"];
      std::vector<WasmEdge::ComponentValVariant> CompParams;
      for (auto A : CompArgs) {
        CompParams.push_back(parseComponentValue(A));
      }
      if (auto Res = onCompInvoke(Ctx, CompModName, std::string(CompField),
                                  CompParams)) {
        if (Res->size() != Expected.size()) {
          EXPECT_NE(LineNumber, LineNumber);
          return;
        }
        size_t I = 0;
        for (auto E : Expected) {
          EXPECT_TRUE(
              equalComponentValue(parseComponentValue(E), (*Res)[I].first));
          ++I;
        }
      } else {
        EXPECT_NE(LineNumber, LineNumber);
      }
      return;
    }
    const auto ModName = GetModuleName(Action);
    const std::string_view Field = Action["field"];
    simdjson::dom::array Args = Action["args"];
    const auto Params = parseValueList(Args);
    const auto Returns = parseExpectedList(Expected);

    // Invoke function of named module. Named modules are registered in Store
    // Manager. Anonymous modules are instantiated in the VM.
    if (auto Res = onInvoke(Ctx, ModName, std::string(Field), Params.first,
                            Params.second)) {
      // Check value.
      EXPECT_TRUE(compares(Returns, *Res));
    } else {
      EXPECT_NE(LineNumber, LineNumber);
    }
  };

  // Helper function to check one of the matched results of invocation.
  auto InvokeEither = [&](const simdjson::dom::object &Action,
                          const simdjson::dom::array &Eithers,
                          uint64_t LineNumber) {
    if (IsComponent) {
      // TODO: Component model invocation not yet supported.
      return;
    }
    const auto ModName = GetModuleName(Action);
    const std::string_view Field = Action["field"];
    simdjson::dom::array Args = Action["args"];
    const auto Params = parseValueList(Args);
    const auto Returns = parseEithersList(Eithers);

    // Invoke function of named module. Named modules are registered in Store
    // Manager. Anonymous modules are instantiated in the VM.
    if (auto Res = onInvoke(Ctx, ModName, std::string(Field), Params.first,
                            Params.second)) {
      // Check value.
      for (auto &Maybe : Returns) {
        if (compares(Maybe, *Res)) {
          return;
        }
      }
      EXPECT_TRUE(compares(Returns[0], *Res))
          << "This is One of available returns.";
    } else {
      EXPECT_NE(LineNumber, LineNumber);
    }
  };

  // Helper function to get values.
  auto Get = [&](const simdjson::dom::object &Action,
                 const simdjson::dom::array &Expected, uint64_t LineNumber) {
    if (IsComponent) {
      // TODO: Component model get not yet supported.
      return;
    }
    const auto ModName = GetModuleName(Action);
    std::string_view Field = Action["field"];
    const auto Returns = parseExpectedList(Expected);

    if (auto Res = onGet(Ctx, ModName, std::string(Field))) {
      // Check value.
      EXPECT_TRUE(compare(Returns[0], *Res));
    } else {
      EXPECT_NE(LineNumber, LineNumber);
    }
  };

  // Helper function to check trap on loading.
  auto TrapLoad = [&](const std::string &FileName, const std::string &Text) {
    if (auto Res = onLoad(Ctx, FileName)) {
      EXPECT_TRUE(false);
    } else {
      EXPECT_TRUE(Res.error().getErrCodePhase() ==
                  WasmEdge::WasmPhase::Loading);
      EXPECT_TRUE(
          stringContains(Text, WasmEdge::ErrCodeStr[Res.error().getEnum()]));
    }
  };

  // Helper function to check trap on validation.
  auto TrapValidate = [&](const std::string &FileName,
                          const std::string &Text) {
    if (auto Res = onValidate(Ctx, FileName); Res) {
      EXPECT_TRUE(false);
    } else {
      EXPECT_TRUE(Res.error().getErrCodePhase() ==
                  WasmEdge::WasmPhase::Validation);
      EXPECT_TRUE(
          stringContains(Text, WasmEdge::ErrCodeStr[Res.error().getEnum()]));
    }
  };

  // Helper function to check trap on instantiation.
  auto TrapInstantiate = [&](const std::string &FileName,
                             const std::string &Text) {
    if (auto Res = onInstantiate(Ctx, FileName); Res) {
      EXPECT_TRUE(false) << "expected instantiation failure: " << FileName;
    } else {
      EXPECT_TRUE(
          Res.error().getErrCodePhase() == WasmEdge::WasmPhase::Instantiation ||
          Res.error().getErrCodePhase() == WasmEdge::WasmPhase::Execution);
      EXPECT_TRUE(
          stringContains(Text, WasmEdge::ErrCodeStr[Res.error().getEnum()]));
    }
  };

  // Helper function to check trap on invocation.
  auto TrapInvoke = [&](const simdjson::dom::object &Action,
                        const std::string &Text, uint64_t LineNumber) {
    if (IsComponent) {
      const auto CompModName = GetModuleName(Action);
      const std::string_view CompField = Action["field"];
      simdjson::dom::array CompArgs = Action["args"];
      std::vector<WasmEdge::ComponentValVariant> CompParams;
      for (auto A : CompArgs) {
        CompParams.push_back(parseComponentValue(A));
      }
      if (auto Res = onCompInvoke(Ctx, CompModName, std::string(CompField),
                                  CompParams)) {
        EXPECT_NE(LineNumber, LineNumber);
      } else {
        EXPECT_TRUE(
            stringContains(Text, WasmEdge::ErrCodeStr[Res.error().getEnum()]));
      }
      return;
    }
    const auto ModName = GetModuleName(Action);
    const std::string_view Field = Action["field"];
    simdjson::dom::array Args = Action["args"];
    const auto Params = parseValueList(Args);

    if (auto Res = onInvoke(Ctx, ModName, std::string(Field), Params.first,
                            Params.second)) {
      EXPECT_NE(LineNumber, LineNumber);
    } else {
      EXPECT_TRUE(Res.error().getErrCodePhase() ==
                  WasmEdge::WasmPhase::Execution);
      EXPECT_TRUE(
          stringContains(Text, WasmEdge::ErrCodeStr[Res.error().getEnum()]));
    }
  };

  // Helper function to check exception on invocation.
  auto ExceptionInvoke = [&](const simdjson::dom::object &Action,
                             uint64_t LineNumber) {
    if (IsComponent) {
      // Component model invocation for exception not yet supported.
      EXPECT_NE(LineNumber, LineNumber);
      return;
    }
    const auto ModName = GetModuleName(Action);
    const std::string_view Field = Action["field"];
    simdjson::dom::array Args = Action["args"];
    const auto Params = parseValueList(Args);

    if (auto Res = onInvoke(Ctx, ModName, std::string(Field), Params.first,
                            Params.second)) {
      EXPECT_NE(LineNumber, LineNumber);
    } else {
      EXPECT_EQ(Res.error(), WasmEdge::ErrCode::Value::UncaughtException);
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
            (TestsuiteRoot / Proposal / UnitName / FileName).u8string();
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
        if (onModule(Ctx, LastModName, FilePath)) {
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
            (TestsuiteRoot / Proposal / UnitName / FileName).u8string();
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
        // Two json schemas exist: {"name","definition"} (wast2json) and
        // {"instance","module"} (wasm-tools json-from-wast).
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
        // Subsequent anonymous invokes target this instance.
        LastModName = ModName;
        if (std::holds_alternative<std::unique_ptr<AST::Component::Component>>(
                ASTDef->second)) {
          auto &ASTComp = *std::get<std::unique_ptr<AST::Component::Component>>(
              ASTDef->second);
          if (onCompInstanceFromDef(Ctx, std::string(ModName), ASTComp)) {
            EXPECT_TRUE(true);
          } else {
            EXPECT_NE(LineNumber, LineNumber);
          }
          return;
        }
        auto &ASTMod = *std::get<std::unique_ptr<AST::Module>>(ASTDef->second);
        if (onInstanceFromDef(Ctx, std::string(ModName), ASTMod)) {
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
          return;
        }
        // A bare `action` runs the invoke for its side effects and ignores
        // any result (no `expected` field); a trap here is a real failure.
        if (IsComponent) {
          const auto CompModName = GetModuleName(Action);
          const std::string_view CompField = Action["field"];
          simdjson::dom::array CompArgs = Action["args"];
          std::vector<WasmEdge::ComponentValVariant> CompParams;
          for (auto A : CompArgs) {
            CompParams.push_back(parseComponentValue(A));
          }
          if (!onCompInvoke(Ctx, CompModName, std::string(CompField),
                            CompParams)) {
            EXPECT_NE(LineNumber, LineNumber);
          }
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
            (TestsuiteRoot / Proposal / UnitName / Name).u8string();
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
            (TestsuiteRoot / Proposal / UnitName / Name).u8string();
        const std::string_view Text = Cmd["text"];
        TrapValidate(Filename, std::string(Text));
        return;
      }
      case CommandID::AssertUnlinkable:
      case CommandID::AssertUninstantiable: {
        const std::string_view Name = Cmd["filename"];
        const auto Filename =
            (TestsuiteRoot / Proposal / UnitName / Name).u8string();
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
