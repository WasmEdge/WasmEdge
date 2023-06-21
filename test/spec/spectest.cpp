// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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
#include "common/log.h"

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
#include "simdjson.h"

namespace {

using namespace std::literals;
using namespace WasmEdge;


// void resolveRegister(std::map<std::string, std::string> &Alias,
//                      simdjson::ondemand::array &CmdArray) {
//   simdjson::ondemand::object ItMod = CmdArray.at(0);
//   CmdArray.reset();
//   std::string_view CmdType;
//   for (auto It: CmdArray) {
//     simdjson::ondemand::object Cmd = It.get_object();
//     if (CmdType == "module"sv) {
//       // Record last module in order.
//       ItMod = Cmd;
//     } else if (CmdType == "register"sv) {
//       std::string_view NewNameStr = Cmd["as"].get_string();
//       std::string_view OrgName;
//       if (auto Org = ItMod.find_field("name")) {
//         OrgName = Org.get_string();
//       }
//       if (auto NameSV = Cmd.find_field("name")) {
//         // Register command records the original name. Set aliasing.
//         std::string_view Name = NameSV.get_string();
//         Alias.emplace(Name.data(), NewNameStr.data());
//       } else {
//         // Register command not records the original name. Get name from the
//         // module. 
//         Alias.emplace(OrgName.data(), NewNameStr.data());
//       }
//     } 
//   }
//}

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
      };
  if (auto Iter = CommandMapping.find(Name); Iter != CommandMapping.end()) {
    return Iter->second;
  }
  return SpecTest::CommandID::Unknown;
}

std::pair<std::vector<WasmEdge::ValVariant>, std::vector<WasmEdge::ValType>>
parseValueList(simdjson::ondemand::array &Args) {
  std::vector<WasmEdge::ValVariant> Result;
  std::vector<WasmEdge::ValType> ResultTypes;
  Result.reserve(Args.count_elements());
  ResultTypes.reserve(Args.count_elements());
  for (auto Element : Args) {
    std::string_view Type;
    auto ValueNode = Element["value"];
    switch (ValueNode.type())
    {
    case simdjson::ondemand::json_type::string:
      {std::string_view Value = ValueNode.get_string();
      if (Type == "externref"sv) {
        if (Value == "null"sv) {
          Result.emplace_back(WasmEdge::UnknownRef());
        } else {
          // Add 0x1 uint32_t prefix in this externref index case.
          Result.emplace_back(WasmEdge::ExternRef(
              reinterpret_cast<void *>(std::stoul(Value.data()) + 0x100000000ULL)));
        }
        ResultTypes.emplace_back(WasmEdge::ValType::ExternRef);
      } else if (Type == "funcref"sv) {
        if (Value == "null"sv) {
          Result.emplace_back(WasmEdge::UnknownRef());
        } else {
          // Add 0x1 uint32_t prefix in this funcref index case.
          Result.emplace_back(WasmEdge::FuncRef(
              reinterpret_cast<WasmEdge::Runtime::Instance::FunctionInstance *>(
                  std::stoul(Value.data()) + 0x100000000ULL)));
        }
        ResultTypes.emplace_back(WasmEdge::ValType::FuncRef);
      } else if (Type == "i32"sv) {
        Result.emplace_back(static_cast<uint32_t>(std::stoul(Value.data())));
        ResultTypes.emplace_back(WasmEdge::ValType::I32);
      } else if (Type == "f32"sv) {
        Result.emplace_back(static_cast<uint32_t>(std::stoul(Value.data())));
        ResultTypes.emplace_back(WasmEdge::ValType::F32);
      } else if (Type == "i64"sv) {
        Result.emplace_back(static_cast<uint64_t>(std::stoull(Value.data())));
        ResultTypes.emplace_back(WasmEdge::ValType::I64);
      } else if (Type == "f64"sv) {
        Result.emplace_back(static_cast<uint64_t>(std::stoull(Value.data())));
        ResultTypes.emplace_back(WasmEdge::ValType::F64);
      } else {
        assumingUnreachable();
      }
      break;}
    case simdjson::ondemand::json_type::array:
      {WasmEdge::uint64x2_t I64x2;
      std::string_view LaneType = Element["lane_type"].get_string();
      simdjson::ondemand::array ValueNodeArray = ValueNode.get_array();
      if (LaneType == "i64"sv || LaneType == "f64"sv) {
        size_t I = 0;
        for (std::string_view X: ValueNodeArray){
          I64x2[I] = std::stoull(X.data());
          I++;
        }
      } else if (LaneType == "i32"sv || LaneType == "f32"sv) {
        using uint32x4_t = uint32_t __attribute__((vector_size(16)));
        uint32x4_t I32x4 = {0};
        size_t I = 0;
        for (std::string_view X: ValueNodeArray){
          I32x4[I] = std::stoull(X.data());
          I++;
        }
        I64x2 = reinterpret_cast<WasmEdge::uint64x2_t>(I32x4);
      } else if (LaneType == "i16"sv) {
        using uint16x8_t = uint16_t __attribute__((vector_size(16)));
        uint16x8_t I16x8 = {0};
        size_t I = 0;
        for (std::string_view X: ValueNodeArray){
          I16x8[I] = std::stoull(X.data());
          I++;
        }
        I64x2 = reinterpret_cast<WasmEdge::uint64x2_t>(I16x8);
      } else if (LaneType == "i8"sv) {
        using uint8x16_t = uint8_t __attribute__((vector_size(16)));
        uint8x16_t I8x16 = {0};
        size_t I = 0;
        for (std::string_view X: ValueNodeArray){
          I8x16[I] = std::stoull(X.data());
          I++;
        }
        I64x2 = reinterpret_cast<WasmEdge::uint64x2_t>(I8x16);
      }
      Result.emplace_back(I64x2);
      ResultTypes.emplace_back(WasmEdge::ValType::V128);
      break;}
    default:
      assumingUnreachable();
      break;
    }
  }
  return {Result, ResultTypes}; 
}

std::vector<std::pair<std::string, std::string>>
parseExpectedList(simdjson::ondemand::array &Args) {
  std::vector<std::pair<std::string, std::string>> Result;
  Result.reserve(Args.count_elements());
  for (auto Element : Args) {
    std::string_view Type = Element["type"].get_string();
    auto ValueNode = Element["value"];
    switch (ValueNode.type())
    {
    case simdjson::ondemand::json_type::string:
      {std::string_view Value = ValueNode.get_string();
      Result.emplace_back(Type.data(), Value);
      break;}
    case simdjson::ondemand::json_type::array:
      {std::string StrValue;
      std::string_view LaneType = Element["lane_type"].get_string();
      simdjson::ondemand::array ValueNodeArray = ValueNode.get_array();
      for (std::string_view X: ValueNodeArray) {
        StrValue += X.data();
        StrValue += ' ';
      }
      StrValue.pop_back();
      std::string FullType = Type.data() + std::string(LaneType.data());
      Result.emplace_back(FullType, std::move(StrValue));
      break;}
    
    default:
      assumingUnreachable();
      break;
    }
  }
  return Result;
}

struct TestsuiteProposal {
  std::string_view Path;
  WasmEdge::Configure Conf;
};

static const TestsuiteProposal TestsuiteProposals[] = {
    {"core"sv, {}},
    {"multi-memory"sv, {Proposal::MultiMemories}},
    {"tail-call"sv, {Proposal::TailCall}},
    {"extended-const"sv, {Proposal::ExtendedConst}},
    {"threads"sv, {Proposal::Threads}},
};

} // namespace

namespace WasmEdge {

std::vector<std::string> SpecTest::enumerate() const {
  std::vector<std::string> Cases;
  for (const auto &Proposal : TestsuiteProposals) {
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
  bool IsV128 = (std::string_view(TypeStr).substr(0, 4) == "v128"sv);
  if (!IsV128 && ValStr.substr(0, 4) == "nan:"sv) {
    // Handle NaN case
    // TODO: nan:canonical and nan:arithmetic
    if (TypeStr == "f32"sv) {
      if (Got.second != ValType::F32) {
        return false;
      }
      return std::isnan(Got.first.get<float>());
    } else if (TypeStr == "f64"sv) {
      if (Got.second != ValType::F64) {
        return false;
      }
      return std::isnan(Got.first.get<double>());
    }
  } else if (TypeStr == "funcref"sv) {
    if (Got.second != ValType::FuncRef) {
      return false;
    }
    if (ValStr == "null"sv) {
      return WasmEdge::isNullRef(Got.first);
    } else {
      if (WasmEdge::isNullRef(Got.first)) {
        return false;
      }
      return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(
                 WasmEdge::retrieveFuncRef(Got.first))) ==
             static_cast<uint32_t>(std::stoul(ValStr));
    }
  } else if (TypeStr == "externref"sv) {
    if (Got.second != ValType::ExternRef) {
      return false;
    }
    if (ValStr == "null"sv) {
      return WasmEdge::isNullRef(Got.first);
    } else {
      if (WasmEdge::isNullRef(Got.first)) {
        return false;
      }
      return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(
                 &WasmEdge::retrieveExternRef<uint32_t>(Got.first))) ==
             static_cast<uint32_t>(std::stoul(ValStr));
    }
  } else if (TypeStr == "i32"sv) {
    if (Got.second != ValType::I32) {
      return false;
    }
    return Got.first.get<uint32_t>() == uint32_t(std::stoul(ValStr));
  } else if (TypeStr == "f32"sv) {
    if (Got.second != ValType::F32) {
      return false;
    }
    // Compare the 32-bit pattern
    return Got.first.get<uint32_t>() == uint32_t(std::stoul(ValStr));
  } else if (TypeStr == "i64"sv) {
    if (Got.second != ValType::I64) {
      return false;
    }
    return Got.first.get<uint64_t>() == uint64_t(std::stoull(ValStr));
  } else if (TypeStr == "f64"sv) {
    if (Got.second != ValType::F64) {
      return false;
    }
    // Compare the 64-bit pattern
    return Got.first.get<uint64_t>() == uint64_t(std::stoull(ValStr));
  } else if (IsV128) {
    std::vector<std::string_view> Parts;
    std::string_view Ev = ValStr;
    if (Got.second != ValType::V128) {
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
      const auto VF = reinterpret_cast<floatx4_t>(V64);
      const auto VI = reinterpret_cast<uint32x4_t>(V64);
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
      const auto VF = reinterpret_cast<doublex2_t>(V64);
      const auto VI = reinterpret_cast<uint64x2_t>(V64);
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
      const auto V = reinterpret_cast<uint8x16_t>(V64);
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
      const auto V = reinterpret_cast<uint16x8_t>(V64);
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
      const auto V = reinterpret_cast<uint32x4_t>(V64);
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

void SpecTest::run(std::string_view Proposal, std::string_view UnitName){
  spdlog::info("{} {}", Proposal, UnitName);
  auto FName = (TestsuiteRoot / Proposal / UnitName /
                       (std::string(UnitName) + ".json"s)).u8string();
  
  simdjson::ondemand::parser parser;
  auto JSON = simdjson::padded_string::load(FName);
  auto Doc = parser.iterate(JSON);

  std::map<std::string, std::string> Alias;
  std::string LastModName;

  // Helper function to get module name.
  auto GetModuleName = [&](simdjson::ondemand::object &Action) -> std::string {
    if (auto Module = Action.find_field("module")) {
      std::string_view ModName = Module.get_string();
      if (auto It = Alias.find(ModName.data()); It != Alias.end()) {
        // If module name is aliased, use the aliased name.
        return It->second;
      }
      return ModName.data();
    }
    return LastModName;
  };

  auto Invoke = [&](simdjson::ondemand::object &Action,
                    simdjson::ondemand::array &Expected, uint64_t LineNumber) {
    const auto ModName = GetModuleName(Action);
    const std::string_view Field = Action["field"].get_string();
    simdjson::ondemand::array Args = Action["args"].get_array();
    const auto Params = parseValueList(Args);
    const auto Returns = parseExpectedList(Expected);

    // Invoke function of named module. Named modules are registered in Store
    // Manager. Anonymous modules are instantiated in VM.
    if (auto Res = onInvoke(ModName, Field.data(), Params.first, Params.second)) {
      // Check value.
      EXPECT_TRUE(compares(Returns, *Res));
    } else {
      EXPECT_NE(LineNumber, LineNumber);
    }
  };

  // Helper function to get values.
  auto Get = [&](simdjson::ondemand::object &Action,
                 simdjson::ondemand::array &Expected, uint64_t LineNumber) {
    const auto ModName = GetModuleName(Action);
    const std::string_view Field = Action["field"].get_string();
    const auto Returns = parseExpectedList(Expected);

    if (auto Res = onGet(ModName, Field.data())) {
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
  auto TrapInvoke = [&](simdjson::ondemand::object &Action, const std::string &Text,
                        uint64_t LineNumber) {
    const auto ModName = GetModuleName(Action);
    const std::string_view Field = Action["field"].get_string();
    simdjson::ondemand::array Args = Action["args"].get_array();
    const auto Params = parseValueList(Args);

    if (auto Res = onInvoke(ModName, Field.data(), Params.first, Params.second)) {
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

  // Command processing. Return true for expected result.
    auto RunCommand = [&](simdjson::ondemand::object &Cmd) {
      // Line number in wast: Cmd["line"].Get<uint32_t>()
      if (auto Type = Cmd.find_field("type")) {
        std::string_view TypeField = Cmd["type"].get_string();
        switch (resolveCommand(TypeField)) {
        case SpecTest::CommandID::Module: {
          std::string_view Name = Cmd["filename"].get_string();
          const auto FileName = (TestsuiteRoot / Proposal / UnitName /
                                Name.data())
                                    .u8string();
          const uint64_t LineNumber = Cmd["line"].get_uint64();
          if (auto Name = Cmd.find_field("name")) {
            // Module has name. Register module with module name.
            std::string_view SVName = Name.get_string();
            LastModName = SVName.data();
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
          simdjson::ondemand::object Action = Cmd["action"].get_object();
          simdjson::ondemand::array Expected = Cmd["expected"].get_array();
          const uint64_t LineNumber = Cmd["line"].get_uint64();
          Invoke(Action, Expected, LineNumber);
          return;
        }
        case CommandID::Register: {
          // Preprocessed. Ignore this.
          return;
        }
        case CommandID::AssertReturn: {
          simdjson::ondemand::object Action = Cmd["action"].get_object();
          simdjson::ondemand::array Expected = Cmd["expected"].get_array();
          std::string_view ActType = Action["type"].get_string();
          const uint64_t LineNumber = Cmd["line"].get_uint64();
          if (ActType == "invoke"sv) {
            Invoke(Action, Expected, LineNumber);
            return;
          } else if (ActType == "get"sv) {
            Get(Action, Expected, LineNumber);
            return;
          }
          EXPECT_TRUE(false);
          return;
        }
        case CommandID::AssertTrap: {
          simdjson::ondemand::object Action = Cmd["action"].get_object();
          std::string_view Text = Cmd["text"].get_string();
          const uint64_t LineNumber = Cmd["line"].get_uint64();
          TrapInvoke(Action, Text.data(), LineNumber);
          return;
        }
        case CommandID::AssertExhaustion: {
          // TODO: Add stack overflow mechanism.
          return;
        }
        case CommandID::AssertMalformed: {
          std::string_view ModType = Cmd["module_type"].get_string();
          std::string Binary = "binary";
          if (Binary.compare(ModType.data())) {
            // TODO: Wat is not supported in WasmEdge yet.
            return;
          }
          std::string_view Name = Cmd["filename"].get_string();
          const auto Filename = (TestsuiteRoot / Proposal / UnitName /
                                Name.data())
                                    .u8string();
          std::string_view Text = Cmd["text"].get_string();
          TrapLoad(Filename, Text.data());
          return;
        }
        case CommandID::AssertInvalid: {
          std::string_view Name = Cmd["filename"].get_string();
          const auto Filename = (TestsuiteRoot / Proposal / UnitName /
                                Name.data())
                                    .u8string();
          std::string_view Text = Cmd["text"].get_string();
          TrapValidate(Filename, Text.data());
          return;
        }
        case CommandID::AssertUnlinkable:
        case CommandID::AssertUninstantiable: {
          std::string_view Name = Cmd["filename"].get_string();
          const auto Filename = (TestsuiteRoot / Proposal / UnitName /
                                Name.data())
                                    .u8string();
          std::string_view Text = Cmd["text"].get_string();
          TrapInstantiate(Filename, Text.data());
          return;
        }
        default:;
        }
      }
      // Unknown command.
      EXPECT_TRUE(false);
    };

  // Get command list.
  
    simdjson::ondemand::array CmdArray = Doc["commands"].get_array();
    // Preprocessing register command.
    //resolveRegister(Alias, CmdArray);

    // Iterate commands.
    for (simdjson::ondemand::object Cmd : CmdArray) {
      try {
        RunCommand(Cmd);
      }
      catch (simdjson::simdjson_error &error) {
        std::cerr << "JSON error: " << error.what() << " near "
                  << __LINE__ << " where: " << FName << std::endl;
      }
    }
}

} // namespace WasmEdge
