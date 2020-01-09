// SPDX-License-Identifier: Apache-2.0
#include "proxy/proxy.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "vm/result.h"
#include "vm/vm.h"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/counting_range.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

namespace SSVM {
namespace Proxy {

/// Helper function for converting string to Executor::Value.
Executor::Value convStrToVal(const std::string &Str, const std::string &Type) {
  Executor::Value Val;
  if (Type == "i32") {
    Val = static_cast<uint32_t>(std::stol(Str));
  } else if (Type == "i64") {
    Val = static_cast<uint64_t>(std::stoll(Str));
  } else if (Type == "f32") {
    Val = std::stof(Str);
  } else if (Type == "f64") {
    Val = std::stod(Str);
  }
  return Val;
}

/// Helper function for converting Executor::Value to string.
std::string convValToStr(const Executor::Value &Val, const std::string &Type) {
  std::string Str;
  if (Type == "i32") {
    Str = std::to_string(static_cast<int32_t>(std::get<uint32_t>(Val)));
  } else if (Type == "i64") {
    Str = std::to_string(static_cast<int64_t>(std::get<uint64_t>(Val)));
  } else if (Type == "f32") {
    Str = boost::lexical_cast<std::string>(std::get<float>(Val));
  } else if (Type == "f64") {
    Str = boost::lexical_cast<std::string>(std::get<double>(Val));
  }
  return Str;
}

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
  if (InputDoc["execution"].FindMember("gas") !=
      InputDoc["execution"].MemberEnd()) {
    OutputDoc["result"]["gas"].SetUint64(
        InputDoc["execution"]["gas"].GetInt64());
  }

  /// Create VM with configure.
  VMUnit.reset();
  VMConf = VM::Configure();
  for (auto &Val : ItModules->value.GetArray()) {
    std::string ModuleType(Val.GetString());
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
  rapidjson::Value::ConstMemberIterator ItArgTypes =
      InputDoc["execution"].FindMember("argument_types");
  rapidjson::Value::ConstMemberIterator ItRetTypes =
      InputDoc["execution"].FindMember("return_types");
  rapidjson::Value::ConstMemberIterator ItVMSnapshot =
      InputDoc["execution"].FindMember("vm_snapshot");
  if (ItGas != InputDoc["execution"].MemberEnd()) {
    /// Set gas.
    VMUnit->setCostLimit(ItGas->value.GetUint64());
  }
  if (ItArgs != InputDoc["execution"].MemberEnd() &&
      ItArgTypes != InputDoc["execution"].MemberEnd()) {
    /// Check arguments and types array.
    if (ItArgs->value.GetArray().Size() !=
        ItArgTypes->value.GetArray().Size()) {
      OutputDoc["result"]["error_message"].SetString(
          "Arguments and types array length not matched.");
      return;
    }
    /// Set start function arguments.
    for (uint32_t I : boost::counting_range(
             size_t(0), size_t(ItArgs->value.GetArray().Size()))) {
      VMUnit->appendArgument(
          convStrToVal(ItArgs->value.GetArray()[I].GetString(),
                       ItArgTypes->value.GetArray()[I].GetString()));
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
  OutputDoc["result"]["gas_used"].SetUint64(VMUnit->getUsedCost());
  VM::Result VMRes = VMUnit->getResult();
  if (VMRes.getState() == VM::Result::State::Commit) {
    OutputDoc["result"]["status"].SetString("Succeeded");
    /// Add return values to output JSON.
    if (ItRetTypes != InputDoc["execution"].MemberEnd() &&
        ItRetTypes->value.GetArray().Size() > 0) {
      /// Store return value.
      std::vector<SSVM::Executor::Value> ReturnVals;
      VMUnit->getReturnValue(ReturnVals);
      /// Check return types and return value.
      if (ReturnVals.size() < ItRetTypes->value.GetArray().Size()) {
        OutputDoc["result"]["error_message"].SetString(
            "Return value array length and wasm function not matched.");
        return;
      }
      for (uint32_t I : boost::counting_range(
               size_t(0), size_t(ItRetTypes->value.GetArray().Size()))) {
        rapidjson::Value ValStr;
        ValStr.SetString(
            convValToStr(ReturnVals[I],
                         ItRetTypes->value.GetArray()[I].GetString())
                .c_str(),
            Allocator);
        OutputDoc["result"]["return_value"].PushBack(ValStr, Allocator);
      }
    }
  } else if (VMRes.getState() == VM::Result::State::Revert) {
    OutputDoc["result"]["status"].SetString("Reverted");
    OutputDoc["result"]["error_message"].SetString("Gas not enough.");
  } else {
    switch (VMRes.getStage()) {
    case VM::Result::Stage::Loader:
      OutputDoc["result"]["error_message"].SetString(
          "Wasm file decoding failed.");
      break;
    case VM::Result::Stage::Validator:
      OutputDoc["result"]["error_message"].SetString("Wasm validation failed.");
      break;
    case VM::Result::Stage::Executor:
      OutputDoc["result"]["error_message"].SetString("Wasm execution failed.");
      break;
    default:
      OutputDoc["result"]["error_message"].SetString("Undefined error.");
      break;
    }
  }
}

void Proxy::exportOutputJSON() {
  rapidjson::StringBuffer StrBuf;
  rapidjson::Writer<rapidjson::StringBuffer> Writer(StrBuf);
  OutputDoc.Accept(Writer);

  std::ofstream OutputFS(OutputJSONPath, std::ios::out | std::ios::trunc);
  if (!OutputFS.is_open()) {
    std::cout << "\n Cannot open output path: \"" << OutputJSONPath << "\""
              << "\n ===================  Results  ==================\n "
              << StrBuf.GetString() << std::endl;
    return;
  }
  OutputFS << StrBuf.GetString();
}

} // namespace Proxy
} // namespace SSVM