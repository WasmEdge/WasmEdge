// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/spec/spectest.cpp - Wasm test suites --------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file parse and run tests of Wasm test suites extracted by wast2json.
/// Test Suits: https://github.com/WebAssembly/testsuite
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#define RAPIDJSON_HAS_STDSTRING 1
#if defined(__SSE4_2__)
#define RAPIDJSON_SSE42 1
#elif defined(__SSE2__)
#define RAPIDJSON_SSE2 1
#elif defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(__ARM_NEON_FP)
#define RAPIDJSON_NEON 1
#endif

#include "spectest.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "gtest/gtest.h"

namespace {

using namespace std::literals;
using namespace SSVM;

/// Preprocessing for set up aliasing.
void resolveRegister(std::map<std::string, std::string> &Alias,
                     rapidjson::Value &CmdArray,
                     rapidjson::Document::AllocatorType &Allocator) {
  rapidjson::Value::ValueIterator ItMod;
  for (rapidjson::Value::ValueIterator It = CmdArray.Begin();
       It != CmdArray.End(); ++It) {
    const auto CmdType = It->GetObject()["type"].Get<std::string>();
    if (CmdType == "module"sv) {
      /// Record last module in order.
      ItMod = It;
    } else if (CmdType == "register"sv) {
      const auto NewName = It->GetObject()["as"].Get<std::string>();
      if (It->GetObject().HasMember("name")) {
        /// Set aliasing.
        Alias.emplace(It->GetObject()["name"].Get<std::string>(), NewName);
      }
      if (auto Name = ItMod->FindMember("name"); Name != ItMod->MemberEnd()) {
        /// Module has origin name. Replace to aliased one.
        Name->value.SetString(NewName, Allocator);
      } else {
        /// Module has no origin name. Add the aliased one.
        rapidjson::Value Text;
        Text.SetString(NewName, Allocator);
        ItMod->AddMember("name", Text, Allocator);
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
      };
  if (auto Iter = CommandMapping.find(Name); Iter != CommandMapping.end()) {
    return Iter->second;
  }
  return SpecTest::CommandID::Unknown;
}

/// Helper function to parse parameters from json to vector of value.
std::vector<SSVM::ValVariant> parseValueList(const rapidjson::Value &Args) {
  std::vector<SSVM::ValVariant> Result;
  Result.reserve(Args.Size());
  for (const auto &Element : Args.GetArray()) {
    const auto &Type = Element["type"].Get<std::string>();
    const auto &ValueNode = Element["value"];
    if (ValueNode.IsString()) {
      const auto &Value = ValueNode.Get<std::string>();
      if (Type == "externref"sv) {
        if (Value == "null"sv) {
          Result.emplace_back(SSVM::genNullRef(SSVM::RefType::ExternRef));
        } else {
          /// Add 0x1 uint32_t prefix in this externref index case.
          Result.emplace_back(SSVM::genExternRef(reinterpret_cast<uint32_t *>(
              std::stoul(Value) + 0x100000000ULL)));
        }
      } else if (Type == "funcref"sv) {
        if (Value == "null"sv) {
          Result.emplace_back(SSVM::genNullRef(SSVM::RefType::FuncRef));
        } else {
          Result.emplace_back(
              SSVM::genFuncRef(static_cast<uint32_t>(std::stoul(Value))));
        }
      } else if (Type == "i32"sv || Type == "f32"sv) {
        Result.emplace_back(static_cast<uint32_t>(std::stoul(Value)));
      } else if (Type == "i64"sv || Type == "f64"sv) {
        Result.emplace_back(static_cast<uint64_t>(std::stoull(Value)));
      } else {
        assert(false);
      }
    } else if (ValueNode.IsArray()) {
      SSVM::uint64x2_t I64x2;
      const auto LaneType = Element["lane_type"].Get<std::string>();
      if (LaneType == "i64"sv || LaneType == "f64"sv) {
        for (size_t I = 0; I < 2; ++I) {
          I64x2[I] = std::stoull(ValueNode[I].Get<std::string>());
        }
      } else if (LaneType == "i32"sv || LaneType == "f32"sv) {
        using uint32x4_t = uint32_t __attribute__((vector_size(16)));
        uint32x4_t I32x4;
        for (size_t I = 0; I < 4; ++I) {
          I32x4[I] = std::stoul(ValueNode[I].Get<std::string>());
        }
        I64x2 = reinterpret_cast<SSVM::uint64x2_t>(I32x4);
      } else if (LaneType == "i16"sv) {
        using uint16x8_t = uint16_t __attribute__((vector_size(16)));
        uint16x8_t I16x8;
        for (size_t I = 0; I < 8; ++I) {
          I16x8[I] = std::stoul(ValueNode[I].Get<std::string>());
        }
        I64x2 = reinterpret_cast<SSVM::uint64x2_t>(I16x8);
      } else if (LaneType == "i8"sv) {
        using uint8x16_t = uint8_t __attribute__((vector_size(16)));
        uint8x16_t I8x16;
        for (size_t I = 0; I < 16; ++I) {
          I8x16[I] = std::stoul(ValueNode[I].Get<std::string>());
        }
        I64x2 = reinterpret_cast<SSVM::uint64x2_t>(I8x16);
      }
      Result.emplace_back(I64x2);
    } else {
      assert(false);
    }
  }
  return Result;
}

/// Helper function to parse parameters from json to vector of string pair.
std::vector<std::pair<std::string, std::string>>
parseExpectedList(const rapidjson::Value &Args) {
  std::vector<std::pair<std::string, std::string>> Result;
  Result.reserve(Args.Size());
  for (const auto &Element : Args.GetArray()) {
    const auto &Type = Element["type"].Get<std::string>();
    const auto &ValueNode = Element["value"];
    if (ValueNode.IsString()) {
      Result.emplace_back(Type, ValueNode.Get<std::string>());
    } else if (ValueNode.IsArray()) {
      const auto LaneType = Element["lane_type"].Get<std::string>();
      std::string Value;
      for (auto Iter = ValueNode.Begin(); Iter != ValueNode.End(); ++Iter) {
        Value += Iter->Get<std::string>();
        Value += ' ';
      }
      Value.pop_back();
      Result.emplace_back(Type + LaneType, std::move(Value));
    } else {
      assert(false);
    }
  }
  return Result;
}

struct TestsuiteProposal {
  std::string_view Path;
  SSVM::ProposalConfigure Conf;
};
static const TestsuiteProposal TestsuiteProposals[] = {
    {"core"sv, {}},
    {"reference-types"sv, {SSVM::Proposal::ReferenceTypes}},
    {"bulk-memory-operations"sv, {SSVM::Proposal::BulkMemoryOperations}},
    {"simd"sv, {SSVM::Proposal::SIMD}},
};

} // namespace

namespace SSVM {

std::vector<std::string> SpecTest::enumerate() const {
  std::vector<std::string> Cases;
  for (const auto Proposal : TestsuiteProposals) {
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

std::tuple<std::string_view, SSVM::ProposalConfigure, std::string>
SpecTest::resolve(std::string_view Params) const {
  const auto Pos = Params.find_last_of(' ');
  const std::string_view ProposalPath = Params.substr(0, Pos);
  const auto Proposal = *std::find_if(std::begin(TestsuiteProposals),
                                      std::end(TestsuiteProposals),
                                      [&ProposalPath](const auto Proposal) {
                                        return Proposal.Path == ProposalPath;
                                      });
  return std::tuple<std::string_view, SSVM::ProposalConfigure, std::string>{
      Proposal.Path, Proposal.Conf, Params.substr(Pos + 1)};
}

void SpecTest::run(std::string_view Proposal, std::string_view UnitName) {
  LOG(INFO) << Proposal << ' ' << UnitName;
  std::ifstream JSONIS(TestsuiteRoot / Proposal / UnitName /
                       (std::string(UnitName) + ".json"s));
  rapidjson::IStreamWrapper JSONISWrapper(JSONIS);
  rapidjson::Document Doc;
  auto &Allocator = Doc.GetAllocator();
  Doc.ParseStream(JSONISWrapper);

  std::map<std::string, std::string> Alias;
  std::string LastModName;

  /// Helper function to get module name.
  auto GetModuleName = [&](const rapidjson::Value &Action) -> std::string {
    if (const auto &Module = Action.FindMember("module"s);
        Module != Action.MemberEnd()) {
      /// Get the module name.
      auto ModName = Module->value.Get<std::string>();
      if (auto It = Alias.find(ModName); It != Alias.end()) {
        /// If module name is aliased, use the aliased name.
        return It->second;
      }
      return ModName;
    }
    return LastModName;
  };

  auto Invoke = [&](const rapidjson::Value &Action,
                    const rapidjson::Value &Expected) {
    const auto ModName = GetModuleName(Action);
    const auto Field = Action["field"s].Get<std::string>();
    const auto Params = parseValueList(Action["args"s]);
    const auto Returns = parseExpectedList(Expected);

    /// Invoke function of named module. Named modules are registered in Store
    /// Manager. Anonymous modules are instantiated in VM.
    if (auto Res = onInvoke(ModName, Field, Params)) {
      /// Check value.
      if (onCompare(Returns, *Res)) {
        EXPECT_TRUE(true);
      } else {
        EXPECT_TRUE(false);
      }
    } else {
      EXPECT_TRUE(false);
    }
  };
  /// Helper function to get values.
  auto Get = [&](const rapidjson::Value &Action,
                 const rapidjson::Value &Expected) {
    const auto ModName = GetModuleName(Action);
    const auto Field = Action["field"s].Get<std::string>();
    const auto Returns = parseExpectedList(Expected);

    if (auto Res = onGet(ModName, Field)) {
      /// Check value.
      EXPECT_TRUE(onCompare(Returns, *Res));
    } else {
      EXPECT_TRUE(false);
    }
  };
  auto TrapInvoke = [&](const rapidjson::Value &Action,
                        const std::string &Text) {
    const auto ModName = GetModuleName(Action);
    const auto Field = Action["field"s].Get<std::string>();
    const auto Params = parseValueList(Action["args"s]);

    if (auto Res = onInvoke(ModName, Field, Params)) {
      EXPECT_TRUE(false);
    } else {
      /// Check value.
      EXPECT_TRUE(onStringContains(Text, SSVM::ErrCodeStr[Res.error()]));
    }
  };
  auto TrapValidate = [&](const std::string &Filename,
                          const std::string &Text) {
    if (auto Res = onValidate(Filename); Res) {
      EXPECT_TRUE(false);
    } else {
      EXPECT_TRUE(onStringContains(Text, SSVM::ErrCodeStr[Res.error()]));
    }
  };
  auto TrapInstantiate = [&](const std::string &Filename,
                             const std::string &Text) {
    if (auto Res = onInstantiate(Filename); Res) {
      EXPECT_TRUE(false);
    } else {
      EXPECT_TRUE(onStringContains(Text, SSVM::ErrCodeStr[Res.error()]));
    }
  };

  /// Command processing. Return true for expected result.
  auto RunCommand = [&](const rapidjson::Value &Cmd) {
    /// Line number in wast: It->GetObject()["line"].GetInt()
    if (const auto Type = Cmd.FindMember("type"s); Type != Cmd.MemberEnd()) {
      switch (resolveCommand(Type->value.Get<std::string>())) {
      case SpecTest::CommandID::Module: {
        const auto FileName = (TestsuiteRoot / Proposal / UnitName /
                               Cmd["filename"].Get<std::string>())
                                  .u8string();
        if (const auto Name = Cmd.FindMember("name"); Name != Cmd.MemberEnd()) {
          /// Module has name. Register module with module name.
          LastModName = Name->value.Get<std::string>();
        } else {
          /// Instantiate the anonymous module.
          LastModName.clear();
        }
        if (onModule(LastModName, FileName)) {
          EXPECT_TRUE(true);
        } else {
          EXPECT_TRUE(false);
        }
        return;
      }
      case CommandID::Action: {
        const auto &Action = Cmd["action"s];
        const auto &Expected = Cmd["expected"s];
        Invoke(Action, Expected);
        return;
      }
      case CommandID::Register: {
        /// Preprocessed. Ignore this.
        return;
      }
      case CommandID::AssertReturn: {
        const auto &Action = Cmd["action"s];
        const auto &Expected = Cmd["expected"s];
        const auto ActType = Action["type"].Get<std::string>();
        if (ActType == "invoke"sv) {
          Invoke(Action, Expected);
          return;
        } else if (ActType == "get"sv) {
          Get(Action, Expected);
          return;
        }
        EXPECT_TRUE(false);
        return;
      }
      case CommandID::AssertTrap: {
        const auto &Action = Cmd["action"s];
        const auto &Text = Cmd["text"s].Get<std::string>();
        TrapInvoke(Action, Text);
        return;
      }
      case CommandID::AssertExhaustion: {
        /// TODO: Add stack overflow mechanism.
        return;
      }
      case CommandID::AssertMalformed: {
        /// TODO: Wat is not supported in SSVM yet.
        /// TODO: Add processing binary cases.
        return;
      }
      case CommandID::AssertInvalid: {
        const auto Filename = (TestsuiteRoot / Proposal / UnitName /
                               Cmd["filename"s].Get<std::string>())
                                  .u8string();
        const auto &Text = Cmd["text"s].Get<std::string>();
        TrapValidate(Filename, Text);
        return;
      }
      case CommandID::AssertUnlinkable:
      case CommandID::AssertUninstantiable: {
        const auto Filename = (TestsuiteRoot / Proposal / UnitName /
                               Cmd["filename"s].Get<std::string>())
                                  .u8string();
        const auto &Text = Cmd["text"s].Get<std::string>();
        TrapInstantiate(Filename, Text);
        return;
      }
      default:;
      }
    }
    /// Unknown command.
    EXPECT_TRUE(false);
  };

  /// Get command list.
  if (auto Commands = Doc.FindMember("commands"s);
      Commands != Doc.MemberEnd()) {
    rapidjson::Value CmdArray;
    CmdArray.CopyFrom(Commands->value, Allocator);

    /// Preprocessing register command.
    resolveRegister(Alias, CmdArray, Allocator);

    /// Iterate commands.
    for (const auto &Cmd : CmdArray.GetArray()) {
      RunCommand(Cmd);
    }
  }
}

} // namespace SSVM
