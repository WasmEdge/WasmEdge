// SPDX-License-Identifier: Apache-2.0
#include "proxy/proxy.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "vm/result.h"
#include "vm/vm.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

namespace SSVM {
namespace Proxy {

void Proxy::runRequest() {
  prepareOutputJSON();
  parseInputJSON();
  executeVM();
  exportOutputJSON();
}

void Proxy::prepareOutputJSON() {
  OutputDoc.SetObject();
  rapidjson::Document::AllocatorType &Allocator = OutputDoc.GetAllocator();
  rapidjson::Value Text;
  Text.SetString("", Allocator);
  OutputDoc.AddMember("service_name", Text, Allocator);
  Text.SetString("0x0", Allocator);
  OutputDoc.AddMember("uuid", Text, Allocator);

  rapidjson::Value ResultObj(rapidjson::kObjectType);
  ResultObj.AddMember("status", "Failed", Allocator);
  ResultObj.AddMember("error_message", "", Allocator);
  ResultObj.AddMember("gas", 0, Allocator);
  ResultObj.AddMember("gas_used", 0, Allocator);
  ResultObj.AddMember("vm_snapshot", rapidjson::Value(rapidjson::kObjectType),
                      Allocator);
  ResultObj.AddMember("return_value", rapidjson::Value(rapidjson::kArrayType),
                      Allocator);
  OutputDoc.AddMember("result", ResultObj, Allocator);
}

void Proxy::parseInputJSON() {
  /// Open JSON file
  std::ifstream InputFS(InputJSONPath, std::ios::binary);
  if (!InputFS.is_open()) {
    OutputDoc["result"]["error_message"].SetString(
        "Input JSON file not found.");
    return;
  }
  InputFS.unsetf(std::ios::skipws);

  /// Get file size
  InputFS.seekg(0, std::ios::end);
  std::streampos FileEndPos = InputFS.tellg();
  InputFS.seekg(0, std::ios::beg);
  uint32_t FileSize = FileEndPos - InputFS.tellg();

  /// Read data to RapidJSON
  std::vector<char> Data;
  Data.reserve(FileSize + 1);
  Data.insert(Data.begin(), std::istream_iterator<char>(InputFS),
              std::istream_iterator<char>());
  Data.push_back(0);
  InputDoc.Parse(&Data[0]);

  /// Parse header
  rapidjson::Value::ConstMemberIterator ItServiceName =
      InputDoc.FindMember("service_name");
  rapidjson::Value::ConstMemberIterator ItUUID = InputDoc.FindMember("uuid");
  rapidjson::Value::ConstMemberIterator ItModules =
      InputDoc.FindMember("modules");
  rapidjson::Value::ConstMemberIterator ItExecution =
      InputDoc.FindMember("execution");
  if (ItModules == InputDoc.MemberEnd() ||
      ItExecution == InputDoc.MemberEnd()) {
    OutputDoc["result"]["error_message"].SetString("Module not determined.");
    return;
  }
  rapidjson::Document::AllocatorType &Allocator = OutputDoc.GetAllocator();
  if (ItServiceName != InputDoc.MemberEnd()) {
    OutputDoc["service_name"].SetString(ItServiceName->value.GetString(),
                                        Allocator);
  }
  if (ItUUID != InputDoc.MemberEnd()) {
    OutputDoc["uuid"].SetString(ItUUID->value.GetString(), Allocator);
  }

  /// Create VM with configure.
  for (auto It = ItModules->value.Begin(); It != ItModules->value.End(); ++It) {
    std::string ModuleType(It->GetString());
    if (ModuleType == "Rust") {
      VMConf.addVMType(SSVM::VM::Configure::VMType::Wasi);
    } else if (ModuleType == "ethereum") {
      OutputDoc["result"]["error_message"].SetString(
          "Ethereum mode is not supported in SSVM-RPC.");
      return;
      /// VMConf.addVMType(SSVM::VM::Configure::VMType::Ewasm);
    }
  }
  VMUnit = std::make_unique<VM::VM>(VMConf);
}

void Proxy::executeVM() {
  /// Wasm path is empty or not found.
  if (WasmPath == "" || !boost::filesystem::exists(WasmPath)) {
    OutputDoc["result"]["error_message"].SetString("Wasm file not found.");
    return;
  }

  /// Failed in previous functions.
  if (VMUnit == nullptr) {
    return;
  }

  /// Set up VM identification.
  VMUnit->getServiceName() = OutputDoc["service_name"].GetString();
  VMUnit->getUUID() = std::strtoull(OutputDoc["uuid"].GetString(), nullptr, 16);

  /// Set up VM execution.
  rapidjson::Value::ConstMemberIterator ItFuncName =
      InputDoc["execution"].FindMember("function_name");
  rapidjson::Value::ConstMemberIterator ItGas =
      InputDoc["execution"].FindMember("gas");
  rapidjson::Value::ConstMemberIterator ItArgs =
      InputDoc["execution"].FindMember("argument");
  rapidjson::Value::ConstMemberIterator ItVMSnapshot =
      InputDoc["execution"].FindMember("vm_snapshot");
  if (ItGas != InputDoc["execution"].MemberEnd()) {
    /// Set gas.
    VMUnit->setCostLimit(ItGas->value.GetUint64());
  }
  if (ItArgs != InputDoc["execution"].MemberEnd()) {
    /// Set start function arguments.
    for (auto It = ItArgs->value.Begin(); It != ItArgs->value.End(); ++It) {
      VMUnit->appendArgument(
          static_cast<uint64_t>(std::strtoull(It->GetString(), nullptr, 16)));
    }
  }
  rapidjson::Document::AllocatorType &Allocator = OutputDoc.GetAllocator();
  if (ItVMSnapshot == InputDoc["execution"].MemberEnd()) {
    InputDoc["execution"].AddMember("vm_snapshot",
                                    rapidjson::Value(rapidjson::kObjectType),
                                    InputDoc.GetAllocator());
  }
  VMUnit->setVMStore(InputDoc["execution"]["vm_snapshot"],
                     OutputDoc["result"]["vm_snapshot"], Allocator);

  /// Execute function.
  VMUnit->setPath(WasmPath);
  if (ItFuncName != InputDoc["execution"].MemberEnd()) {
    VMUnit->execute(ItFuncName->value.GetString());
  } else {
    VMUnit->execute();
  }

  /// Add VM result to output JSON.
  OutputDoc["result"]["gas"].SetUint64(VMUnit->getCostLimit());
  OutputDoc["result"]["gas_used"].SetUint64(VMUnit->getUsedCost());
  std::vector<SSVM::Executor::Value> ReturnVals;
  VMUnit->getReturnValue(ReturnVals);
  for (auto It = ReturnVals.cbegin(); It != ReturnVals.cend(); It++) {
    uint64_t val = std::get<uint64_t>(*It);
    rapidjson::Value ValStr;
    std::string ValHex =
        (boost::format("0x%016llx") % std::get<uint64_t>(*It)).str();
    ValStr.SetString(ValHex.c_str(), Allocator);
    OutputDoc["result"]["return_value"].PushBack(ValStr, Allocator);
  }
  VM::Result VMRes = VMUnit->getResult();
  if (VMRes.getState() == VM::Result::State::Commit) {
    OutputDoc["result"]["status"].SetString("Succeeded", Allocator);
  } else if (VMRes.getState() == VM::Result::State::Revert) {
    OutputDoc["result"]["status"].SetString("Reverted", Allocator);
  }

  VMUnit.reset();
  VMConf = VM::Configure();
}

void Proxy::exportOutputJSON() {
  rapidjson::StringBuffer StrBuf;
  rapidjson::Writer<rapidjson::StringBuffer> Writer(StrBuf);
  OutputDoc.Accept(Writer);

  std::ofstream OutputFS(OutputJSONPath, std::ios::out | std::ios::trunc);
  if (!OutputFS.is_open()) {
    std::cout << "Cannot open output path: \"" << OutputJSONPath << "\""
              << "\n------------ Result ------------\n"
              << StrBuf.GetString() << std::endl;
    return;
  }
  OutputFS << StrBuf.GetString();
}

} // namespace Proxy
} // namespace SSVM