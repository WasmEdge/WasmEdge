// SPDX-License-Identifier: Apache-2.0
#include "proxy/proxy.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/range/counting_range.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

namespace SSVM {
namespace Proxy {

/// Resume from JSON.
Expect<void> restore(Runtime::StoreManager &StoreMgr,
                     const rapidjson::Value &Doc) {
  /// Get instantiated active module instance.
  Runtime::Instance::ModuleInstance *ModInst;
  if (auto Res = StoreMgr.getActiveModule()) {
    ModInst = *Res;
  } else {
    return Unexpect(Res);
  }
  /// Find Global instances.
  rapidjson::Value::ConstMemberIterator ItGlob = Doc.FindMember("global");
  if (ItGlob != Doc.MemberEnd()) {
    for (auto It = ItGlob->value.Begin(); It != ItGlob->value.End(); ++It) {
      /// Set global with value.
      const uint32_t Idx = It->GetArray()[0].GetUint();
      if (auto Res = ModInst->getGlobalAddr(Idx)) {
        auto *GlobInst = *StoreMgr.getGlobal(*Res);
        ValVariant Val = static_cast<uint64_t>(
            std::stoull(It->GetArray()[1].GetString(), 0, 16));
        GlobInst->getValue() = Val;
      } else {
        return Unexpect(Res);
      }
    }
  }

  /// Find Memory instances.
  rapidjson::Value::ConstMemberIterator ItMem = Doc.FindMember("memory");
  if (ItMem != Doc.MemberEnd()) {
    for (auto It = ItMem->value.Begin(); It != ItMem->value.End(); ++It) {
      /// Get memory address and data
      const uint32_t Idx = It->GetArray()[0].GetUint();
      const std::string &Hex = It->GetArray()[1].GetString();
      Runtime::Instance::MemoryInstance *MemInst = nullptr;
      if (auto Res = ModInst->getMemAddr(Idx)) {
        MemInst = *StoreMgr.getMemory(*Res);
      } else {
        return Unexpect(Res);
      }
      Bytes MemVec;
      boost::algorithm::unhex(Hex.begin(), Hex.end(),
                              std::back_inserter(MemVec));
      if (auto Res = MemInst->setBytes(MemVec, 0, 0, MemVec.size()); !Res) {
        return Unexpect(Res);
      }
    }
  }
  return {};
}

/// Snapshot to JSON.
Expect<void> snapshot(Runtime::StoreManager &StoreMgr, rapidjson::Value &Doc,
                      rapidjson::Document::AllocatorType &Alloc) {
  /// Get instantiated active module instance.
  Runtime::Instance::ModuleInstance *ModInst;
  if (auto Res = StoreMgr.getActiveModule()) {
    ModInst = *Res;
  } else {
    return Unexpect(Res);
  }

  /// Iterate Global instances.
  if (ModInst->getGlobalNum() > 0) {
    rapidjson::Value GlobArr(rapidjson::kArrayType);
    for (uint32_t I = 0; I < ModInst->getGlobalNum(); ++I) {
      /// Get address and data to string.
      uint32_t GlobAddr = *ModInst->getGlobalAddr(I);
      auto *GlobInst = *StoreMgr.getGlobal(GlobAddr);
      ValVariant Val = GlobInst->getValue();
      std::string ValHex =
          (boost::format("0x%016llx") % retrieveValue<uint64_t>(Val)).str();

      /// Insert into global array.
      rapidjson::Value GlobData(rapidjson::kArrayType);
      rapidjson::Value Idx(I);
      rapidjson::Value ValStr;
      ValStr.SetString(ValHex.c_str(), Alloc);
      GlobData.PushBack(Idx, Alloc);
      GlobData.PushBack(ValStr, Alloc);
      GlobArr.PushBack(GlobData, Alloc);
    }
    Doc.AddMember("global", GlobArr, Alloc);
  }

  /// Iterate Memory instances.
  if (ModInst->getMemNum() > 0) {
    rapidjson::Value MemArr(rapidjson::kArrayType);
    for (uint32_t I = 0; I < ModInst->getMemNum(); ++I) {
      /// Get address and data to string.
      uint32_t MemAddr = *ModInst->getMemAddr(I);
      auto *MemInst = *StoreMgr.getMemory(MemAddr);
      const std::vector<uint8_t> &Data = MemInst->getDataVector();
      std::string DataHex;
      boost::algorithm::hex_lower(Data.begin(), Data.end(),
                                  std::back_inserter(DataHex));

      /// Insert into memory array.
      if (DataHex.length() > 0) {
        rapidjson::Value MemData(rapidjson::kArrayType);
        rapidjson::Value Idx(I);
        rapidjson::Value MemStr;
        MemStr.SetString(DataHex.c_str(), Alloc);
        MemData.PushBack(Idx, Alloc);
        MemData.PushBack(MemStr, Alloc);
        MemArr.PushBack(MemData, Alloc);
      }
    }
    if (MemArr.Size() > 0) {
      Doc.AddMember("memory", MemArr, Alloc);
    }
  }
  return {};
}

/// Helper function for converting string to ValVariant.
ValVariant convStrToVal(const std::string &Str, const std::string &Type) {
  ValVariant Val;
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

/// Helper function for converting ValVariant to string.
std::string convValToStr(const ValVariant &Val, const std::string &Type) {
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
  VMConf = ExpVM::Configure();
  for (auto &Val : ItModules->value.GetArray()) {
    std::string ModuleType(Val.GetString());
    if (ModuleType == "Rust") {
      VMConf.addVMType(SSVM::ExpVM::Configure::VMType::Wasi);
    } else if (ModuleType == "ethereum") {
      OutputDoc["result"]["error_message"].SetString(
          "Ethereum mode is not supported in SSVM-RPC.");
      return;
      /// VMConf.addVMType(SSVM::VM::Configure::VMType::Ewasm);
    }
  }
  VMUnit = std::make_unique<ExpVM::VM>(VMConf);
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
  std::vector<ValVariant> Params, Results;
  rapidjson::Document::AllocatorType &Allocator = OutputDoc.GetAllocator();
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
  if (ItGas != InputDoc["execution"].MemberEnd()) {
    /// Set gas.
    VMUnit->getMeasurement().getCostLimit() = ItGas->value.GetUint64();
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
      Params.emplace_back(
          convStrToVal(ItArgs->value.GetArray()[I].GetString(),
                       ItArgTypes->value.GetArray()[I].GetString()));
    }
  }

  /// Instantiate wasm module.
  ErrCode Status = ErrCode::Success;
  if (auto Res = VMUnit->loadWasm(WasmPath); !Res) {
    Status = Res.error();
    OutputDoc["result"]["error_message"].SetString("Wasm decoding failed.");
  }
  if (Status == ErrCode::Success) {
    if (auto Res = VMUnit->validate(); !Res) {
      Status = Res.error();
      OutputDoc["result"]["error_message"].SetString("Wasm validation failed.");
    }
  }
  if (Status == ErrCode::Success) {
    if (auto Res = VMUnit->instantiate(); !Res) {
      Status = Res.error();
      OutputDoc["result"]["error_message"].SetString(
          "Wasm instantiation failed.");
    }
  }

  /// Restore VM state.
  if (Status == ErrCode::Success &&
      InputDoc["execution"].FindMember("vm_snapshot") !=
          InputDoc["execution"].MemberEnd()) {
    if (auto Res = restore(VMUnit->getStoreManager(),
                           InputDoc["execution"]["vm_snapshot"]);
        !Res) {
      Status = Res.error();
    }
  }

  /// Execute function.
  if (Status == ErrCode::Success) {
    std::string FuncName;
    if (ItFuncName != InputDoc["execution"].MemberEnd()) {
      FuncName = ItFuncName->value.GetString();
    }
    if (auto Res = VMUnit->execute(FuncName, Params)) {
      OutputDoc["result"]["gas_used"].SetUint64(
          VMUnit->getMeasurement().getCostSum());
      if (ItRetTypes != InputDoc["execution"].MemberEnd() &&
          ItRetTypes->value.GetArray().Size() > 0) {
        /// Check return types is match.
        Results = *Res;
        if (Results.size() != ItRetTypes->value.GetArray().Size()) {
          OutputDoc["result"]["error_message"].SetString(
              "Return value array length and wasm function not matched.");
          return;
        }
        /// Write return values.
        for (uint32_t I : boost::counting_range(
                 size_t(0), size_t(ItRetTypes->value.GetArray().Size()))) {
          rapidjson::Value ValStr;
          ValStr.SetString(
              convValToStr(Results[I],
                           ItRetTypes->value.GetArray()[I].GetString())
                  .c_str(),
              Allocator);
          OutputDoc["result"]["return_value"].PushBack(ValStr, Allocator);
        }
      }
      OutputDoc["result"]["status"].SetString("Succeeded");
    } else {
      Status = Res.error();
      if (Status == ErrCode::Revert) {
        OutputDoc["result"]["status"].SetString("Reverted");
        OutputDoc["result"]["error_message"].SetString("Reverted by Ewasm.");
      } else {
        OutputDoc["result"]["error_message"].SetString(
            "Wasm execution failed.");
      }
    }
  }

  /// Snapshot VM
  if (Status == ErrCode::Success) {
    snapshot(VMUnit->getStoreManager(), OutputDoc["result"]["vm_snapshot"],
             Allocator);
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