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
#include "common/spdlog.h"

#include "simdjson.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <gtest/gtest.h>
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
    bool Replaced = false;
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
      } else if (!OrgName.empty()) {
        // Register command does not record the original name. Get name from the
        // module.
        Replaced = true;
        Alias.emplace(std::string(OrgName), std::string(NewNameStr));
      }
      if (!OrgName.empty() && !Replaced) {
        // Module has origin name. Replace to aliased one.
        Alias.emplace(std::string(OrgName), NewNameStr);
      } else {
        // Module has no origin name. Add the aliased one.
        Alias.emplace(std::to_string(LastModLine), NewNameStr);
      }
    }
  }
}

SpecTest::CommandID resolveCommand(std::string_view Name) {
  static const std::unordered_map<std::string_view, SpecTest::CommandID>
      CommandMapping = {
          {"module"sv, SpecTest::CommandID::Module},
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
      WasmEdge::uint64x2_t I64x2;
      std::string_view LaneType = Element["lane_type"];
      if (LaneType == "i64"sv || LaneType == "f64"sv) {
        size_t I = 0;
        for (std::string_view X : ValueNodeArray) {
          I64x2[I] = std::stoull(std::string(X));
          I++;
        }
      } else if (LaneType == "i32"sv || LaneType == "f32"sv) {
        using uint32x4_t = SIMDArray<uint32_t, 16>;
        uint32x4_t I32x4 = {0};
        size_t I = 0;
        for (std::string_view X : ValueNodeArray) {
          I32x4[I] = static_cast<uint32_t>(std::stoull(std::string(X)));
          I++;
        }
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
        I64x2 = reinterpret_cast<WasmEdge::uint64x2_t &>(I32x4);
#else
        I64x2 = reinterpret_cast<WasmEdge::uint64x2_t>(I32x4);
#endif

      } else if (LaneType == "i16"sv) {
        using uint16x8_t = SIMDArray<uint16_t, 16>;
        uint16x8_t I16x8 = {0};
        size_t I = 0;
        for (std::string_view X : ValueNodeArray) {
          I16x8[I] = static_cast<uint16_t>(std::stoull(std::string(X)));
          I++;
        }
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
        I64x2 = reinterpret_cast<WasmEdge::uint64x2_t &>(I16x8);
#else
        I64x2 = reinterpret_cast<WasmEdge::uint64x2_t>(I16x8);
#endif
      } else if (LaneType == "i8"sv) {
        using uint8x16_t = SIMDArray<uint8_t, 16>;
        uint8x16_t I8x16 = {0};
        size_t I = 0;
        for (std::string_view X : ValueNodeArray) {
          I8x16[I] = static_cast<uint8_t>(std::stoull(std::string(X)));
          I++;
        }
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
        I64x2 = reinterpret_cast<WasmEdge::uint64x2_t &>(I8x16);
#else
        I64x2 = reinterpret_cast<WasmEdge::uint64x2_t>(I8x16);
#endif
      } else {
        assumingUnreachable();
      }
      Result.emplace_back(I64x2);
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
  std::string_view Path;
  WasmEdge::Configure Conf;
  WasmEdge::SpecTest::TestMode Mode = WasmEdge::SpecTest::TestMode::All;
};

static const TestsuiteProposal TestsuiteProposals[] = {
    {"core"sv, {}},
    {"multi-memory"sv, {Proposal::MultiMemories}},
    {"tail-call"sv, {Proposal::TailCall}},
    {"extended-const"sv, {Proposal::ExtendedConst}},
    {"threads"sv, {Proposal::Threads}},
    {"function-references"sv,
     {Proposal::FunctionReferences, Proposal::TailCall}},
    {"gc"sv, {Proposal::GC}, WasmEdge::SpecTest::TestMode::Interpreter},
    {"exception-handling"sv,
     {Proposal::ExceptionHandling, Proposal::TailCall},
     WasmEdge::SpecTest::TestMode::Interpreter},
    // LEGACY-EH: remove the legacy EH test after deprecating legacy EH.
    {"exception-handling-legacy"sv,
     {Proposal::ExceptionHandling, Proposal::TailCall},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"relaxed-simd"sv, {Proposal::RelaxSIMD}},
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
    if (!Got.second.isRefType() ||
        Got.second.getHeapTypeCode() != TypeCode::ExnRef) {
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
      const uint64x2_t V64 = {
          static_cast<uint64_t>(Got.first.get<uint128_t>()),
          static_cast<uint64_t>(Got.first.get<uint128_t>() >> 64U)};
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
      const auto VF = reinterpret_cast<const floatx4_t &>(V64);
      const auto VI = reinterpret_cast<const uint32x4_t &>(V64);
#else
      const auto VF = reinterpret_cast<floatx4_t>(V64);
      const auto VI = reinterpret_cast<uint32x4_t>(V64);
#endif
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
      const uint64x2_t V64 = {
          static_cast<uint64_t>(Got.first.get<uint128_t>()),
          static_cast<uint64_t>(Got.first.get<uint128_t>() >> 64U)};
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
      const auto VF = reinterpret_cast<const doublex2_t &>(V64);
      const auto VI = reinterpret_cast<const uint64x2_t &>(V64);
#else
      const auto VF = reinterpret_cast<doublex2_t>(V64);
      const auto VI = reinterpret_cast<uint64x2_t>(V64);
#endif
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
      const uint64x2_t V64 = {
          static_cast<uint64_t>(Got.first.get<uint128_t>()),
          static_cast<uint64_t>(Got.first.get<uint128_t>() >> 64U)};
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
      const auto V = reinterpret_cast<const uint8x16_t &>(V64);
#else
      const auto V = reinterpret_cast<uint8x16_t>(V64);
#endif
      for (size_t I = 0; I < 16; ++I) {
        const uint8_t V1 = V[I];
        const uint8_t V2 =
            static_cast<uint8_t>(std::stoul(std::string(Parts[I])));
        if (V1 != V2) {
          return false;
        }
      }
    } else if (LaneType == "i16") {
      const uint64x2_t V64 = {
          static_cast<uint64_t>(Got.first.get<uint128_t>()),
          static_cast<uint64_t>(Got.first.get<uint128_t>() >> 64U)};
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
      const auto V = reinterpret_cast<const uint16x8_t &>(V64);
#else
      const auto V = reinterpret_cast<uint16x8_t>(V64);
#endif
      for (size_t I = 0; I < 8; ++I) {
        const uint16_t V1 = V[I];
        const uint16_t V2 =
            static_cast<uint16_t>(std::stoul(std::string(Parts[I])));
        if (V1 != V2) {
          return false;
        }
      }
    } else if (LaneType == "i32") {
      const uint64x2_t V64 = {
          static_cast<uint64_t>(Got.first.get<uint128_t>()),
          static_cast<uint64_t>(Got.first.get<uint128_t>() >> 64U)};
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
      const auto V = reinterpret_cast<const uint32x4_t &>(V64);
#else
      const auto V = reinterpret_cast<uint32x4_t>(V64);
#endif
      for (size_t I = 0; I < 4; ++I) {
        const uint32_t V1 = V[I];
        const uint32_t V2 = std::stoul(std::string(Parts[I]));
        if (V1 != V2) {
          return false;
        }
      }
    } else if (LaneType == "i64") {
      const uint64x2_t V = {
          static_cast<uint64_t>(Got.first.get<uint128_t>()),
          static_cast<uint64_t>(Got.first.get<uint128_t>() >> 64U)};
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
    spdlog::error("   ##### expected text : {}", Expected);
    spdlog::error("   ######## error text : {}", Got);
    return false;
  }
  return true;
}

void SpecTest::run(std::string_view Proposal, std::string_view UnitName) {
  spdlog::info("{} {}", Proposal, UnitName);
  auto TestFileName =
      (TestsuiteRoot / Proposal / UnitName / (std::string(UnitName) + ".json"s))
          .string();

  simdjson::dom::parser Parser;
  simdjson::dom::element Doc = Parser.load(TestFileName);

  std::map<std::string, std::string> Alias;
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
  auto TrapLoad = [&](const std::string &Filename, const std::string &Text) {
    if (auto Res = onLoad(Filename)) {
      EXPECT_TRUE(false);
    } else {
      EXPECT_TRUE(
          stringContains(Text, WasmEdge::ErrCodeStr[Res.error().getEnum()]));
    }
  };
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
      // Check value.
      EXPECT_TRUE(
          stringContains(Text, WasmEdge::ErrCodeStr[Res.error().getEnum()]));
    }
  };
  auto TrapValidate = [&](const std::string &Filename,
                          const std::string &Text) {
    if (auto Res = onValidate(Filename); Res) {
      EXPECT_TRUE(false);
    } else {
      EXPECT_TRUE(
          stringContains(Text, WasmEdge::ErrCodeStr[Res.error().getEnum()]));
    }
  };
  auto TrapInstantiate = [&](const std::string &Filename,
                             const std::string &Text) {
    if (auto Res = onInstantiate(Filename); Res) {
      EXPECT_TRUE(false);
    } else {
      EXPECT_TRUE(
          stringContains(Text, WasmEdge::ErrCodeStr[Res.error().getEnum()]));
    }
  };
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
        std::string_view Name = Cmd["filename"];
        const auto FileName =
            (TestsuiteRoot / Proposal / UnitName / Name).u8string();
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
        if (onModule(LastModName, FileName)) {
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
        simdjson::dom::array Expected, Either;

        if (Cmd["expected"].get(Expected) == simdjson::error_code::SUCCESS) {
          if (ActType == "invoke"sv) {
            Invoke(Action, Expected, LineNumber);
            return;
          } else if (ActType == "get"sv) {
            Get(Action, Expected, LineNumber);
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
