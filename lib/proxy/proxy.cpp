// SPDX-License-Identifier: Apache-2.0
#include "proxy/proxy.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <boost/format.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

namespace SSVM {
namespace Proxy {

void Proxy::runRequest() {
  /// No output JSON Path. Do nothing.
  if (OutputJSONPath == "") {
    return;
  }

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
  ResultObj.AddMember("Gas", 0, Allocator);
  ResultObj.AddMember("UsedGas", 0, Allocator);
  ResultObj.AddMember("Error_Message", "", Allocator);
  OutputDoc.AddMember("Result", ResultObj, Allocator);
}

void Proxy::parseInputJSON() {
  /// Open JSON file
  std::ifstream InputFS(InputJSONPath, std::ios::binary);
  if (!InputFS.is_open()) {
    rapidjson::Document::AllocatorType &Alloc = OutputDoc.GetAllocator();
    OutputDoc["Result"]["Error_Message"].SetString(
        "Input JSON file not found.");
    return;
  }
  InputFS.unsetf(std::ios::skipws);

  /// Get file size
  InputFS.seekg(0, std::ios::end);
  std::streampos InputFileSize = InputFS.tellg();
  InputFS.seekg(0, std::ios::beg);

  /// Read data into vector
  std::vector<char> Data;
  Data.reserve(InputFileSize);
  Data.insert(Data.begin(), std::istream_iterator<char>(InputFS),
              std::istream_iterator<char>());

  /// Create rapidjson objects
  rapidjson::Document Doc;
  Doc.Parse(&Data[0]);

  /// Parse header
  rapidjson::Value::ConstMemberIterator ItServiceName =
      Doc.FindMember("Service_Name");
  rapidjson::Value::ConstMemberIterator ItUUID = Doc.FindMember("UUID");
  rapidjson::Value::ConstMemberIterator ItModules = Doc.FindMember("Modules");
  rapidjson::Value::ConstMemberIterator ItExecution =
      Doc.FindMember("Execution");
  if (ItModules == Doc.MemberEnd() || ItExecution == Doc.MemberEnd()) {
    OutputDoc["Result"]["Error_Message"].SetString("Module not determined.");
    return;
  }

  /// Create VM with configure.
  VM::Configure Conf;
  for (auto It = ItModules->value.Begin(); It != ItModules->value.End(); ++It) {
    std::string ModuleType(It->GetString());
    if (ModuleType == "Rust") {
      Conf.addVMType(SSVM::VM::Configure::VMType::Wasi);
    } else if (ModuleType == "Ethereum") {
      Conf.addVMType(SSVM::VM::Configure::VMType::Ewasm);
    }
  }
  VMUnit = std::make_unique<VM::VM>(Conf);
  if (ItServiceName != Doc.MemberEnd()) {
    VMUnit->getServiceName() = ItServiceName->value.GetString();
  }
  if (ItUUID != Doc.MemberEnd()) {
    VMUnit->getUUID() = std::stoull(ItUUID->value.GetString(), nullptr, 16);
  }
}

void Proxy::executeVM() {
  /// TODO: Set up execution.

  /// TODO: Run wasm function.

  /// Add VM indentification to output JSON.
  rapidjson::Document::AllocatorType &Allocator = OutputDoc.GetAllocator();
  std::string Hex = (boost::format("0x%016llx") % VMUnit->getUUID()).str();
  OutputDoc["Service_Name"].SetString(VMUnit->getServiceName().c_str(),
                                      Allocator);
  OutputDoc["UUID"].SetString(Hex.c_str(), Allocator);

  /// Add VM result to output JSON.
  OutputDoc["Result"]["Gas"].SetUint64(VMUnit->getCostLimit());
  OutputDoc["Result"]["UsedGas"].SetUint64(VMUnit->getUsedCost());
  /// TODO: Add VM snapshot and return value.
}

void Proxy::exportOutputJSON() {
  rapidjson::StringBuffer StrBuf;
  rapidjson::Writer<rapidjson::StringBuffer> Writer(StrBuf);
  OutputDoc.Accept(Writer);

  std::ofstream OutputFS(OutputJSONPath, std::ios::out | std::ios::trunc);
  if (!OutputFS.is_open()) {
    std::cout << "Cannot open output path: " << OutputJSONPath << std::endl;
    return;
  }
  OutputFS << StrBuf.GetString();
}

} // namespace Proxy
} // namespace SSVM