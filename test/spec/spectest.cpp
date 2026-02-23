// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/spec/spectest.cpp - Wasm test suites ----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file parse and run tests of Wasm test suites extracted by wast2json.
/// Test Suits: https://github.com/WebAssembly/testsuite
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
#include <unordered_map>

namespace {

using namespace std::literals;
using namespace WasmEdge;

// Preprocessing for set up aliasing.
void resolveRegister(std::map<std::string, std::string> &Alias,
                     simdjson::dom::array &CmdArray) {
  std::string_view OrgName;
  uint64_t LastModLine = 0;
  for (const simdjson::dom::object &Cmd : CmdArray) {
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

// Helper function to parse parameters from json to vector of value.
std::pair<std::vector<WasmEdge::ValVariant>, std::vector<WasmEdge::ValType>>
parseValueList(const simdjson::dom::array &Args) {
  std::vector<WasmEdge::ValVariant> Result;
  std::vector<WasmEdge::ValType> ResultTypes;
  Result.reserve(Args.size());
  ResultTypes.reserve(Args.size());
  for (const simdjson::dom::object &Element : Args) {
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
          // Not support input value of opaque references for testing.
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

// Helper function to parse parameters from json to vector of string pair.
std::vector<std::pair<std::string, std::string>>
parseExpectedList(const simdjson::dom::array &Args) {
  std::vector<std::pair<std::string, std::string>> Result;
  Result.reserve(Args.size());
  for (const simdjson::dom::object &Element : Args) {
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
    // TODO: MEMORY64 - proposal not implemented and OFF by default currently.
    // Should turn on and remove this column below after implementation ready.
    {"wasm-3.0"sv, WasmEdge::Standard::WASM_3, {Proposal::Memory64}},
    {"wasm-3.0-bulk-memory"sv, WasmEdge::Standard::WASM_3},
    // TODO: EXCEPTION - implement the AOT.
    {"wasm-3.0-exceptions"sv,
     WasmEdge::Standard::WASM_3,
     {},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"wasm-3.0-gc"sv, WasmEdge::Standard::WASM_3},
    // TODO: MEMORY64 - Turn on this test.
    // {"wasm-3.0-memory64"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-multi-memory"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-relaxed-simd"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-simd"sv, WasmEdge::Standard::WASM_3},
    {"threads"sv, WasmEdge::Standard::WASM_2, {Proposal::Threads}},
};

} // namespace

namespace WasmEdge {

std::vector<std::string>
SpecTest::enumerate(const SpecTest::TestMode Mode) const {
  std::vector<std::string> Cases;
  for (const auto &Proposal : TestsuiteProposals) {
    if (static_cast<uint8_t>(Proposal.Mode) & static_cast<uint8_t>(Mode)) {
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
          const uint32_t V2 = std::stoul(std::string(Parts[I]));
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
        const uint32_t V2 = std::stoul(std::string(Parts[I]));
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

  std::map<std::string, std::string> Alias;
  std::map<std::string, std::unique_ptr<AST::Module>> ASTMap;
  std::string LastModName;

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

  // Helper function to check result of invocation.
  auto Invoke = [&](const simdjson::dom::object &Action,
                    const simdjson::dom::array &Expected, uint64_t LineNumber) {
    const auto ModName = GetModuleName(Action);
    const std::string_view Field = Action["field"];
    simdjson::dom::array Args = Action["args"];
    const auto Params = parseValueList(Args);
    const auto Returns = parseExpectedList(Expected);

    // Invoke function of named module. Named modules are registered in Store
    // Manager. Anonymous modules are instantiated in VM.
    if (auto Res = onInvoke(ModName, std::string(Field), Params.first,
                            Params.second)) {
      // Check value.
      EXPECT_TRUE(compares(Returns, *Res));
    } else {
      EXPECT_NE(LineNumber, LineNumber);
    }
  };

  // Helper function to check one of matched results of invocation.
  auto InvokeEither = [&](const simdjson::dom::object &Action,
                          const simdjson::dom::array &Eithers,
                          uint64_t LineNumber) {
    const auto ModName = GetModuleName(Action);
    const std::string_view Field = Action["field"];
    simdjson::dom::array Args = Action["args"];
    const auto Params = parseValueList(Args);
    const auto Returns = parseEithersList(Eithers);

    // Invoke function of named module. Named modules are registered in Store
    // Manager. Anonymous modules are instantiated in VM.
    if (auto Res = onInvoke(ModName, std::string(Field), Params.first,
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
    const auto ModName = GetModuleName(Action);
    std::string_view Field = Action["field"];
    const auto Returns = parseExpectedList(Expected);

    if (auto Res = onGet(ModName, std::string(Field))) {
      // Check value.
      EXPECT_TRUE(compare(Returns[0], *Res));
    } else {
      EXPECT_NE(LineNumber, LineNumber);
    }
  };

  // Helper function to check trap on loading.
  auto TrapLoad = [&](const std::string &FileName, const std::string &Text) {
    if (auto Res = onLoad(FileName)) {
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
    if (auto Res = onValidate(FileName); Res) {
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
    if (auto Res = onInstantiate(FileName); Res) {
      EXPECT_TRUE(false);
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
    const auto ModName = GetModuleName(Action);
    const std::string_view Field = Action["field"];
    simdjson::dom::array Args = Action["args"];
    const auto Params = parseValueList(Args);

    if (auto Res = onInvoke(ModName, std::string(Field), Params.first,
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
    const auto ModName = GetModuleName(Action);
    const std::string_view Field = Action["field"];
    simdjson::dom::array Args = Action["args"];
    const auto Params = parseValueList(Args);

    if (auto Res = onInvoke(ModName, std::string(Field), Params.first,
                            Params.second)) {
      EXPECT_NE(LineNumber, LineNumber);
    } else {
      EXPECT_EQ(Res.error(), WasmEdge::ErrCode::Value::UncaughtException);
    }
  };

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
        if (onModule(LastModName, FilePath)) {
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
        if (auto Res = onModuleDefine(std::string(FilePath)); Res) {
          if (!Cmd["name"].get(ASTName)) {
            // Module definition has name. Store the AST module.
            ASTMap.emplace(std::string(ASTName), std::move(*Res));
          }
          // TODO: maybe should handle the anonymous AST module in the future.
          EXPECT_TRUE(true);
        } else {
          EXPECT_NE(LineNumber, LineNumber);
        }
        return;
      }
      case CommandID::ModuleInstance: {
        std::string_view ModName = Cmd["name"];
        std::string_view ASTName = Cmd["definition"];
        const uint64_t LineNumber = Cmd["line"];
        auto ASTDef = ASTMap.find(std::string(ASTName));
        if (ASTDef == ASTMap.end()) {
          EXPECT_NE(LineNumber, LineNumber);
          return;
        }
        if (auto It = Alias.find(std::string(ModName)); It != Alias.end()) {
          ModName = It->second;
        }
        if (onInstanceFromDef(std::string(ModName), *(ASTDef->second).get())) {
          EXPECT_TRUE(true);
        } else {
          EXPECT_NE(LineNumber, LineNumber);
        }
        return;
      }
      case CommandID::Action: {
        const simdjson::dom::object &Action = Cmd["action"];
        const simdjson::dom::array &Expected = Cmd["expected"];
        const uint64_t LineNumber = Cmd["line"];
        Invoke(Action, Expected, LineNumber);
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
        const simdjson::dom::object &Action = Cmd["action"];
        const uint64_t LineNumber = Cmd["line"];
        const auto ModName = GetModuleName(Action);
        const std::string_view Field = Action["field"];
        simdjson::dom::array Args = Action["args"];
        const auto Params = parseValueList(Args);

        if (auto Res = onInvoke(ModName, std::string(Field), Params.first,
                                Params.second)) {
          EXPECT_NE(LineNumber, LineNumber);
        } else {
          EXPECT_TRUE(Res.error().getErrCodePhase() ==
                      WasmEdge::WasmPhase::Execution);
          EXPECT_EQ(Res.error(), WasmEdge::ErrCode::Value::CallStackExhausted);
        }
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
        // TODO: Check expected exception type
        if (ActType == "invoke"sv) {
          ExceptionInvoke(Action, LineNumber);
          return;
        }
        EXPECT_TRUE(false);
        return;
      }
      default:;
      }
    }
    // Unknown command.
    EXPECT_TRUE(false);
  };

  // Get command list.
  simdjson::dom::array CmdArray;

  if (!Doc["commands"].get(CmdArray)) {
    // Preprocessing register command.
    resolveRegister(Alias, CmdArray);

    // Iterate commands.
    for (const simdjson::dom::object &Cmd : CmdArray) {
      RunCommand(Cmd);
    }
  }
}

} // namespace WasmEdge
