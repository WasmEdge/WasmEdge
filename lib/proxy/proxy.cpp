// SPDX-License-Identifier: Apache-2.0
#include "proxy/proxy.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "vm/result.h"
#include "vm/vm.h"

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
  OutputDoc.AddMember("Service_Name", Text, Allocator);
  Text.SetString("0x0", Allocator);
  OutputDoc.AddMember("UUID", Text, Allocator);

  rapidjson::Value ResultObj(rapidjson::kObjectType);
  ResultObj.AddMember("Status", "Failed", Allocator);
  ResultObj.AddMember("Error_Message", "", Allocator);
  ResultObj.AddMember("Gas", 0, Allocator);
  ResultObj.AddMember("UsedGas", 0, Allocator);
  ResultObj.AddMember("VMSnapshot", rapidjson::Value(rapidjson::kObjectType),
                      Allocator);
  ResultObj.AddMember("ReturnValue", rapidjson::Value(rapidjson::kArrayType),
                      Allocator);
  OutputDoc.AddMember("Result", ResultObj, Allocator);
}

void Proxy::parseInputJSON() {
  /// Open JSON file
  std::ifstream InputFS(InputJSONPath, std::ios::binary);
  if (!InputFS.is_open()) {
    OutputDoc["Result"]["Error_Message"].SetString(
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
      InputDoc.FindMember("Service_Name");
  rapidjson::Value::ConstMemberIterator ItUUID = InputDoc.FindMember("UUID");
  rapidjson::Value::ConstMemberIterator ItModules =
      InputDoc.FindMember("Modules");
  rapidjson::Value::ConstMemberIterator ItExecution =
      InputDoc.FindMember("Execution");
  if (ItModules == InputDoc.MemberEnd() ||
      ItExecution == InputDoc.MemberEnd()) {
    OutputDoc["Result"]["Error_Message"].SetString("Module not determined.");
    return;
  }
  rapidjson::Document::AllocatorType &Allocator = OutputDoc.GetAllocator();
  if (ItServiceName != InputDoc.MemberEnd()) {
    OutputDoc["Service_Name"].SetString(ItServiceName->value.GetString(),
                                        Allocator);
  }
  if (ItUUID != InputDoc.MemberEnd()) {
    OutputDoc["UUID"].SetString(ItUUID->value.GetString(), Allocator);
  }

  /// Create VM with configure.
  for (auto It = ItModules->value.Begin(); It != ItModules->value.End(); ++It) {
    std::string ModuleType(It->GetString());
    if (ModuleType == "Rust") {
      VMConf.addVMType(SSVM::VM::Configure::VMType::Wasi);
    } else if (ModuleType == "Ethereum") {
      OutputDoc["Result"]["Error_Message"].SetString(
          "Ethereum mode is not supported in SSVM-RPC.");
      return;
      /// VMConf.addVMType(SSVM::VM::Configure::VMType::Ewasm);
    }
  }
  VMUnit = std::make_unique<VM::VM>(VMConf);
}

void Proxy::executeVM() {
  /// Wasm path is empty or not found.
  if (WasmPath == "") {
    OutputDoc["Result"]["Error_Message"].SetString("Wasm file not found.");
    return;
  }

  /// Failed in previous functions.
  if (VMUnit == nullptr) {
    return;
  }

  /// Set up VM identification.
  VMUnit->getServiceName() = OutputDoc["Service_Name"].GetString();
  VMUnit->getUUID() = std::strtoull(OutputDoc["UUID"].GetString(), nullptr, 16);

  /// Set up VM execution.
  rapidjson::Value::ConstMemberIterator ItFuncName =
      InputDoc["Execution"].FindMember("Function_Name");
  rapidjson::Value::ConstMemberIterator ItGas =
      InputDoc["Execution"].FindMember("Gas");
  rapidjson::Value::ConstMemberIterator ItArgs =
      InputDoc["Execution"].FindMember("Argument");
  rapidjson::Value::ConstMemberIterator ItVMSnapshot =
      InputDoc["Execution"].FindMember("VMSnapshot");
  if (ItGas != InputDoc["Execution"].MemberEnd()) {
    /// Set gas.
    VMUnit->setCostLimit(ItGas->value.GetUint64());
  }
  if (ItArgs != InputDoc["Execution"].MemberEnd()) {
    /// Set start function arguments.
    for (auto It = ItArgs->value.Begin(); It != ItArgs->value.End(); ++It) {
      VMUnit->appendArgument(
          static_cast<uint64_t>(std::strtoull(It->GetString(), nullptr, 16)));
    }
  }
  rapidjson::Document::AllocatorType &Allocator = OutputDoc.GetAllocator();
  if (ItVMSnapshot == InputDoc["Execution"].MemberEnd()) {
    InputDoc["Execution"].AddMember("VMSnapshot",
                                    rapidjson::Value(rapidjson::kObjectType),
                                    InputDoc.GetAllocator());
  }
  VMUnit->setVMStore(InputDoc["Execution"]["VMSnapshot"],
                     OutputDoc["Result"]["VMSnapshot"], Allocator);

  /// Execute function.
  VMUnit->setPath(WasmPath);
  if (ItFuncName != InputDoc["Execution"].MemberEnd()) {
    VMUnit->execute(ItFuncName->value.GetString());
  } else {
    VMUnit->execute();
  }

  /// Add VM result to output JSON.
  OutputDoc["Result"]["Gas"].SetUint64(VMUnit->getCostLimit());
  OutputDoc["Result"]["UsedGas"].SetUint64(VMUnit->getUsedCost());
  std::vector<SSVM::Executor::Value> ReturnVals;
  VMUnit->getReturnValue(ReturnVals);
  for (auto It = ReturnVals.cbegin(); It != ReturnVals.cend(); It++) {
    uint64_t val = std::get<uint64_t>(*It);
    rapidjson::Value ValStr;
    std::string ValHex =
        (boost::format("0x%016llx") % std::get<uint64_t>(*It)).str();
    ValStr.SetString(ValHex.c_str(), Allocator);
    OutputDoc["Result"]["ReturnValue"].PushBack(ValStr, Allocator);
  }
  VM::Result VMRes = VMUnit->getResult();
  if (VMRes.getState() == VM::Result::State::Commit) {
    OutputDoc["Result"]["Status"].SetString("Succeeded", Allocator);
  } else if (VMRes.getState() == VM::Result::State::Revert) {
    OutputDoc["Result"]["Status"].SetString("Reverted", Allocator);
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