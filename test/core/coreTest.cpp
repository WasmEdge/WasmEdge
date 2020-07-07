// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/core/coreTest.cpp - Wasm test suites --------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents tests of Wasm test suites extracted by wast2json.
/// Test Suits: https://github.com/WebAssembly/spec/tree/master/test/core
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "spectest.h"
#include "support/filesystem.h"
#include "support/log.h"
#include "vm/configure.h"
#include "vm/vm.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>

namespace {

const std::string TestSuiteRoot = "testSuites";

/// Helper function to compare return values with json.
bool compareValList(rapidjson::Value::ConstMemberIterator &ItRets,
                    const std::vector<SSVM::ValVariant> &Vals) {
  if (ItRets->value.GetArray().Size() != Vals.size()) {
    return false;
  }
  auto ItObj = ItRets->value.GetArray().begin();
  auto ItVal = Vals.begin();
  for (; ItObj != ItRets->value.GetArray().end(); ++ItObj, ++ItVal) {
    std::string Type = ItObj->GetObject()["type"].GetString();
    std::string Value = ItObj->GetObject()["value"].GetString();
    /// Handle NaN case
    if (Value.substr(0, 4) == "nan:") {
      /// TODO: nan:canonical and nan:arithmetic
      if (Type == "f32") {
        float F = std::get<float>(*ItVal);
        if (!std::isnan(F)) {
          return false;
        }
      } else if (Type == "f64") {
        double D = std::get<double>(*ItVal);
        if (!std::isnan(D)) {
          return false;
        }
      }
    } else if (Type == "i32" || Type == "f32") {
      uint32_t V1 = std::get<uint32_t>(*ItVal);
      uint32_t V2 = static_cast<uint32_t>(std::stoul(Value));
      if (V1 != V2) {
        return false;
      }
    } else {
      uint64_t V1 = std::get<uint64_t>(*ItVal);
      uint64_t V2 = static_cast<uint64_t>(std::stoull(Value));
      if (V1 != V2) {
        return false;
      }
    }
  }
  return true;
}

/// Helper function to parse parameters from json to vector.
std::vector<SSVM::ValVariant>
parseValList(rapidjson::Value::ConstMemberIterator &ItArgs) {
  std::vector<SSVM::ValVariant> Vals;
  for (rapidjson::Value::ConstValueIterator It =
           ItArgs->value.GetArray().begin();
       It != ItArgs->value.GetArray().end(); It++) {
    std::string Type = It->GetObject()["type"].GetString();
    std::string Value = It->GetObject()["value"].GetString();
    if (Type == "i32" || Type == "f32") {
      Vals.emplace_back(static_cast<uint32_t>(std::stoul(Value)));
    } else {
      Vals.emplace_back(static_cast<uint64_t>(std::stoull(Value)));
    }
  }
  return Vals;
}

/// Helper function to call functions.
bool expectInvoke(std::map<std::string, std::string> &Alias,
                  std::string &LastModName, SSVM::VM::VM &VM,
                  rapidjson::Value::ConstMemberIterator ItAct,
                  rapidjson::Value::ConstMemberIterator ItExp) {
  std::string ModName = LastModName;
  std::string Field = "";
  Field.assign(ItAct->value["field"].GetString(),
               ItAct->value["field"].GetStringLength());
  rapidjson::Value::ConstMemberIterator ItArgs =
      ItAct->value.FindMember("args");

  if (ItAct->value.FindMember("module") != ItAct->value.MemberEnd()) {
    /// Get the module name.
    ModName = ItAct->value["module"].GetString();
    auto It = Alias.find(ModName);
    if (It != Alias.end()) {
      /// If module name is aliased, use the aliased name.
      ModName = It->second;
    }
  }

  auto Params = parseValList(ItArgs);
  std::vector<SSVM::ValVariant> Rets;
  if (ModName == "") {
    /// Invoke function of anonymous module. Anonymous modules are
    /// instantiated in VM.
    if (auto Res = VM.execute(Field, Params)) {
      Rets = *Res;
    } else {
      return false;
    }
  } else {
    /// Invoke function of named module. Named modules are registered in
    /// Store Manager.
    if (auto Res = VM.execute(ModName, Field, Params)) {
      Rets = *Res;
    } else {
      return false;
    }
  }
  return compareValList(ItExp, Rets);
}

/// Helper function to get values.
bool expectGet(std::map<std::string, std::string> &Alias,
               std::string &LastModName, SSVM::VM::VM &VM,
               rapidjson::Value::ConstMemberIterator ItAct,
               rapidjson::Value::ConstMemberIterator ItExp) {
  std::string ModName = LastModName;
  std::string Field = "";
  Field.assign(ItAct->value["field"].GetString(),
               ItAct->value["field"].GetStringLength());

  if (ItAct->value.FindMember("module") != ItAct->value.MemberEnd()) {
    /// Get the module name.
    ModName = ItAct->value["module"].GetString();
    auto It = Alias.find(ModName);
    if (It != Alias.end()) {
      /// If module name is aliased, use the aliased name.
      ModName = It->second;
    }
  }

  /// Get module instance.
  auto &Store = VM.getStoreManager();
  SSVM::Runtime::Instance::ModuleInstance *ModInst = nullptr;
  if (ModName == "") {
    ModInst = *Store.getActiveModule();
  } else {
    if (auto Res = Store.findModule(ModName)) {
      ModInst = *Res;
    }
  }
  if (ModInst == nullptr) {
    return false;
  }

  /// Get global instance.
  auto &Globs = ModInst->getGlobalExports();
  if (Globs.find(Field) == Globs.cend()) {
    return false;
  }
  uint32_t GlobAddr = Globs.find(Field)->second;
  auto *GlobInst = *Store.getGlobal(GlobAddr);

  /// Check value.
  std::vector<SSVM::ValVariant> Vals = {GlobInst->getValue()};
  return compareValList(ItExp, Vals);
}

/// Helper function to call functions.
bool unexpectInvoke(std::map<std::string, std::string> &Alias,
                    std::string &LastModName, SSVM::VM::VM &VM,
                    rapidjson::Value::ConstMemberIterator ItAct,
                    rapidjson::Value::ConstMemberIterator ItMsg) {
  std::string ModName = LastModName;
  std::string Field = "";
  Field.assign(ItAct->value["field"].GetString(),
               ItAct->value["field"].GetStringLength());
  rapidjson::Value::ConstMemberIterator ItArgs =
      ItAct->value.FindMember("args");

  if (ItAct->value.FindMember("module") != ItAct->value.MemberEnd()) {
    /// Get the module name.
    ModName = ItAct->value["module"].GetString();
    auto It = Alias.find(ModName);
    if (It != Alias.end()) {
      /// If module name is aliased, use the aliased name.
      ModName = It->second;
    }
  }

  auto Params = parseValList(ItArgs);
  SSVM::Expect<std::vector<SSVM::ValVariant>> Res;
  SSVM::ErrCode Code;
  if (ModName == "") {
    /// Invoke function of anonymous module. Anonymous modules are
    /// instantiated in VM.
    Res = VM.execute(Field, Params);
  } else {
    /// Invoke function of named module. Named modules are registered in
    /// Store Manager.
    Res = VM.execute(ModName, Field, Params);
  }

  if (Res) {
    return false;
  } else {
    Code = static_cast<SSVM::ErrCode>(Res.error());
  }

  if (SSVM::ErrCodeStr[Code].rfind(ItMsg->value.GetString(), 0) != 0) {
    std::cout << "   ##### expected text : " << ItMsg->value.GetString()
              << std::endl;
    std::cout << "   ######## error text : " << SSVM::ErrCodeStr[Code]
              << std::endl;
    return false;
  }
  return true;
}

/// Helper function to validate modules
bool unexpectValidate(std::string &FileName, SSVM::VM::VM &VM,
                      rapidjson::Value::ConstMemberIterator ItMsg) {
  SSVM::ErrCode Code;
  if (!VM.loadWasm(FileName)) {
    return false;
  }
  if (auto Res = VM.validate()) {
    return false;
  } else {
    Code = static_cast<SSVM::ErrCode>(Res.error());
  }

  if (SSVM::ErrCodeStr[Code].rfind(ItMsg->value.GetString(), 0) != 0) {
    std::cout << "   ##### expected text : " << ItMsg->value.GetString()
              << std::endl;
    std::cout << "   ######## error text : " << SSVM::ErrCodeStr[Code]
              << std::endl;
    return false;
  }
  return true;
}

/// Helper function to instantiate modules
bool unexpectInstantiate(std::string &FileName, SSVM::VM::VM &VM,
                         rapidjson::Value::ConstMemberIterator ItMsg) {
  SSVM::ErrCode Code;
  if (!VM.loadWasm(FileName)) {
    return false;
  }
  if (!VM.validate()) {
    return false;
  }
  if (auto Res = VM.instantiate()) {
    return false;
  } else {
    Code = static_cast<SSVM::ErrCode>(Res.error());
  }

  if (SSVM::ErrCodeStr[Code].rfind(ItMsg->value.GetString(), 0) != 0) {
    std::cout << "   ##### expected text : " << ItMsg->value.GetString()
              << std::endl;
    std::cout << "   ######## error text : " << SSVM::ErrCodeStr[Code]
              << std::endl;
    return false;
  }
  return true;
}

/// Parameterized testing class.
class CoreTest : public testing::TestWithParam<std::string> {};

/// Preprocessing for set up aliasing.
void resolveRegister(std::map<std::string, std::string> &Alias,
                     rapidjson::Value::MemberIterator ItCmds,
                     rapidjson::Document::AllocatorType &Allocator) {
  rapidjson::Value::ValueIterator ItMod;
  for (rapidjson::Value::ValueIterator It = ItCmds->value.GetArray().begin();
       It != ItCmds->value.GetArray().end(); It++) {
    std::string CmdType = It->GetObject()["type"].GetString();
    if (CmdType == "module") {
      /// Record last module in order.
      ItMod = It;
    } else if (CmdType == "register") {
      std::string NewName = It->GetObject()["as"].GetString();
      if (It->GetObject().HasMember("name")) {
        /// Set aliasing.
        Alias.insert({It->GetObject()["name"].GetString(), NewName});
      }
      if (ItMod->GetObject().HasMember("name")) {
        /// Module has origin name. Replace to aliased one.
        ItMod->GetObject()["name"].SetString(NewName.c_str(), Allocator);
      } else {
        /// Module has no origin name. Add the aliased one.
        rapidjson::Value Text;
        Text.SetString(NewName.c_str(), Allocator);
        ItMod->GetObject().AddMember("name", Text, Allocator);
      }
    }
  }
}

/// Command processing. Return true for expected result.
bool TestCommand(const std::string &UnitName, std::string &LastModName,
                 std::map<std::string, std::string> &Alias, SSVM::VM::VM &VM,
                 rapidjson::Value::ConstValueIterator ItCmd) {
  std::string CmdType = ItCmd->GetObject()["type"].GetString();
  if (CmdType == "module") {
    std::string FileName = TestSuiteRoot + "/" + UnitName + "/" +
                           ItCmd->GetObject()["filename"].GetString();
    if (ItCmd->GetObject().FindMember("name") !=
        ItCmd->GetObject().MemberEnd()) {
      /// Module has name. Register module with module name.
      if (!VM.registerModule(ItCmd->GetObject()["name"].GetString(),
                             FileName)) {
        return false;
      }
      LastModName = ItCmd->GetObject()["name"].GetString();
    } else {
      /// Instantiate the anonymous module.
      if (!VM.loadWasm(FileName)) {
        return false;
      }
      if (!VM.validate()) {
        return false;
      }
      if (!VM.instantiate()) {
        return false;
      }
      LastModName = "";
    }
    return true;
  } else if (CmdType == "action") {
    rapidjson::Value::ConstMemberIterator ItAction =
        ItCmd->GetObject().FindMember("action");
    rapidjson::Value::ConstMemberIterator ItExpect =
        ItCmd->GetObject().FindMember("expected");
    return expectInvoke(Alias, LastModName, VM, ItAction, ItExpect);
  } else if (CmdType == "register") {
    /// Preprocessed. Ignore this.
    return true;
  } else if (CmdType == "assert_return") {
    rapidjson::Value::ConstMemberIterator ItAction =
        ItCmd->GetObject().FindMember("action");
    rapidjson::Value::ConstMemberIterator ItExpect =
        ItCmd->GetObject().FindMember("expected");
    std::string ActType = ItAction->value["type"].GetString();
    if (ActType == "invoke") {
      return expectInvoke(Alias, LastModName, VM, ItAction, ItExpect);
    } else if (ActType == "get") {
      return expectGet(Alias, LastModName, VM, ItAction, ItExpect);
    }
    return false;
  } else if (CmdType == "assert_trap") {
    rapidjson::Value::ConstMemberIterator ItAction =
        ItCmd->GetObject().FindMember("action");
    rapidjson::Value::ConstMemberIterator ItText =
        ItCmd->GetObject().FindMember("text");

    return unexpectInvoke(Alias, LastModName, VM, ItAction, ItText);
  } else if (CmdType == "assert_exhaustion") {
    /// TODO: Add stack overflow mechanism.
    return true;
  } else if (CmdType == "assert_malformed") {
    /// TODO: Wat is not supported in SSVM yet.
    /// TODO: Add processing binary cases.
    return true;
  } else if (CmdType == "assert_invalid") {
    std::string FileName = TestSuiteRoot + "/" + UnitName + "/" +
                           ItCmd->GetObject()["filename"].GetString();
    rapidjson::Value::ConstMemberIterator ItText =
        ItCmd->GetObject().FindMember("text");
    return unexpectValidate(FileName, VM, ItText);
  } else if (CmdType == "assert_unlinkable" ||
             CmdType == "assert_uninstantiable") {
    std::string FileName = TestSuiteRoot + "/" + UnitName + "/" +
                           ItCmd->GetObject()["filename"].GetString();
    rapidjson::Value::ConstMemberIterator ItText =
        ItCmd->GetObject().FindMember("text");
    return unexpectInstantiate(FileName, VM, ItText);
  } else {
    /// Unknown command.
    return false;
  }
}

TEST_P(CoreTest, TestSuites) {
  const std::string UnitName = GetParam();
  SSVM::VM::Configure Conf;
  SSVM::VM::VM VM(Conf);
  SSVM::Host::SpecTestModule SpecTestMod;
  VM.registerModule(SpecTestMod);
  std::map<std::string, std::string> Alias;
  std::string LastModName = "";

  /// Parse test unit json file.
  std::ifstream JSONIS(TestSuiteRoot + "/" + UnitName + "/" + UnitName +
                       ".json");
  rapidjson::IStreamWrapper JSONISWrapper(JSONIS);
  rapidjson::Document Doc;
  Doc.ParseStream(JSONISWrapper);

  /// Get command list.
  if (Doc.HasMember("commands")) {
    /// Preprocessing register command.
    resolveRegister(Alias, Doc.FindMember("commands"), Doc.GetAllocator());

    /// Iterate commands.
    for (rapidjson::Value::ConstValueIterator It =
             Doc["commands"].GetArray().Begin();
         It != Doc["commands"].GetArray().End(); ++It) {
      /// Line number in wast: It->GetObject()["line"].GetInt()
      EXPECT_TRUE(TestCommand(UnitName, LastModName, Alias, VM, It));
    }
  }
}

/// Initiate test suite.
std::vector<std::string> prepareTestDir(const std::string &TestRootPath) {
  std::vector<std::string> Vec;
  for (const auto &TestUnit :
       std::filesystem::directory_iterator(TestRootPath)) {
    for (const auto &File : std::filesystem::directory_iterator(TestUnit)) {
      if (boost::algorithm::ends_with(File.path().string(), ".json")) {
        Vec.push_back(TestUnit.path().filename().string());
      }
    }
  }
  std::sort(Vec.begin(), Vec.end());
  return Vec;
}
INSTANTIATE_TEST_SUITE_P(TestUnit, CoreTest,
                         testing::ValuesIn(prepareTestDir(TestSuiteRoot)));

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  SSVM::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}