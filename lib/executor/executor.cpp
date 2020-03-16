// SPDX-License-Identifier: Apache-2.0
#include "executor/executor.h"
#include "ast/module.h"
#include "ast/section.h"
#include "executor/instance/module.h"
#include "support/log.h"

#include <boost/algorithm/hex.hpp>
#include <boost/format.hpp>
#include <cstdlib>
#include <iostream>

namespace SSVM {
namespace Executor {

/// Set and instantiate host function. See "include/executor/executor.h".
ErrCode Executor::setHostFunction(std::unique_ptr<HostFunctionBase> &Func) {
  ErrCode Status = ErrCode::Success;
  auto NewFuncInst = std::make_unique<Instance::FunctionInstance>(true);
  unsigned int NewHostFuncId = 0;
  unsigned int NewFuncInstId = 0;

  /// Set function instance data.
  if ((Status = NewFuncInst->setNames(
           Func->getModName(), Func->getFuncName())) != ErrCode::Success) {
    return Status;
  }
  if ((Status = NewFuncInst->setFuncType(Func->getFuncType())) !=
      ErrCode::Success) {
    return Status;
  }

  /// Insert host function to host function manager.
  if ((Status = HostFuncMgr.insertHostFunction(Func, NewHostFuncId)) !=
      ErrCode::Success) {
    return Status;
  }
  if ((Status = NewFuncInst->setHostFuncAddr(NewHostFuncId)) !=
      ErrCode::Success) {
    return Status;
  }

  /// Insert function instance to store manager.
  if ((Status = StoreMgr.insertFunctionInst(NewFuncInst, NewFuncInstId)) !=
      ErrCode::Success) {
    return Status;
  }
  return Status;
}

/// Set start function name. See "include/executor/executor.h".
ErrCode Executor::setStartFuncName(const std::string &Name) {
  StartFunc = Name;
  return ErrCode::Success;
}

/// Set AST Module node to executor. See "include/executor/executor.h".
ErrCode Executor::setModule(std::unique_ptr<AST::Module> &Module) {
  /// Check is the correct state.
  if (Stat != State::Inited)
    return ErrCode::WrongExecutorFlow;

  /// Get ownership of module.
  Mod = std::move(Module);
  Stat = State::ModuleSet;
  return ErrCode::Success;
}

/// Instantiate module. See "include/executor/executor.h".
ErrCode Executor::instantiate() {
  /// Check is the correct state.
  if (Stat != State::ModuleSet)
    return ErrCode::WrongExecutorFlow;

  /// Instantiate module.
  ErrCode Result = instantiate(Mod.get());
  if (Result == ErrCode::Success)
    Stat = State::Instantiated;
  else {
    // TODO: Unified error output format
    LOG(ERROR) << "Wasm instantiation failed. Code: "
               << static_cast<uint32_t>(Result) << std::endl;
  }
  return Result;
}

/// Set arguments. See "include/executor/executor.h".
ErrCode Executor::setArgs(std::vector<Value> &Args) {
  /// Check is the correct state.
  if (Stat != State::Instantiated)
    return ErrCode::WrongExecutorFlow;

  /// Push args to stack.
  for (auto It = Args.begin(); It != Args.end(); It++) {
    StackMgr.push(std::move(*It));
  }
  Args.clear();
  Stat = State::ArgsSet;
  return ErrCode::Success;
}

/// Set memory at specific memory index with given bytes array and length
ErrCode Executor::setMemoryWithBytes(const std::vector<uint8_t> &Src,
                                     const uint32_t DistMemIdx,
                                     const uint32_t MemOffset,
                                     const uint64_t Size) {
  Instance::MemoryInstance *MemInst = nullptr;
  if (ErrCode Status = StoreMgr.getMemory(DistMemIdx, MemInst);
      Status != ErrCode::Success) {
    return Status;
  }
  return MemInst->setBytes(const_cast<std::vector<uint8_t> &>(Src), MemOffset,
                           0, Size);
}

/// Get memory and save into bytes array from given memory index and length
ErrCode Executor::getMemoryToBytes(const uint32_t SrcMemIdx,
                                   const uint32_t MemOffset,
                                   std::vector<uint8_t> &Dist,
                                   const uint64_t Size) {
  Instance::MemoryInstance *MemInst = nullptr;
  if (ErrCode Status = StoreMgr.getMemory(SrcMemIdx, MemInst);
      Status != ErrCode::Success) {
    return Status;
  }
  return MemInst->getBytes(Dist, MemOffset, Size);
}

/// Get all memory and save into bytes array from given memory index
ErrCode Executor::getMemoryToBytesAll(const uint32_t SrcMemIdx,
                                      std::vector<uint8_t> &Dist,
                                      unsigned int &DataPageSize) {
  Instance::MemoryInstance *MemInst = nullptr;
  if (ErrCode Status = StoreMgr.getMemory(SrcMemIdx, MemInst);
      Status != ErrCode::Success) {
    return Status;
  }
  DataPageSize = MemInst->getDataPageSize();
  return MemInst->getBytes(Dist, 0, MemInst->getDataVector().size());
}

/// Set memory data page size
ErrCode Executor::setMemoryDataPageSize(const uint32_t SrcMemIdx,
                                        const unsigned int DataPageSize) {
  Instance::MemoryInstance *MemInst = nullptr;
  if (ErrCode Status = StoreMgr.getMemory(SrcMemIdx, MemInst);
      Status != ErrCode::Success) {
    return Status;
  }
  MemInst->setLimit(DataPageSize, false, 0);
  return ErrCode::Success;
}

/// Resume from JSON. See "include/executor/executor.h"
ErrCode Executor::restore(const rapidjson::Value &Doc) {
  /// Find Global instances.
  rapidjson::Value::ConstMemberIterator ItGlob = Doc.FindMember("global");
  if (ItGlob != Doc.MemberEnd()) {
    for (auto It = ItGlob->value.Begin(); It != ItGlob->value.End(); ++It) {
      /// Get global address and hex
      const uint32_t Idx = It->GetArray()[0].GetUint();
      Instance::GlobalInstance *GlobInst = nullptr;
      if (ErrCode Status = StoreMgr.getGlobal(Idx, GlobInst);
          Status != ErrCode::Success) {
        return Status;
      }
      AST::ValVariant Val = static_cast<uint64_t>(
          std::stoull(It->GetArray()[1].GetString(), 0, 16));
      GlobInst->setValue(Val);
    }
  }

  /// Find Memory instances.
  rapidjson::Value::ConstMemberIterator ItMem = Doc.FindMember("memory");
  if (ItMem != Doc.MemberEnd()) {
    for (auto It = ItMem->value.Begin(); It != ItMem->value.End(); ++It) {
      /// Get memory address and data
      const uint32_t Idx = It->GetArray()[0].GetUint();
      const std::string &Hex = It->GetArray()[1].GetString();
      Instance::MemoryInstance *MemInst = nullptr;
      if (ErrCode Status = StoreMgr.getMemory(Idx, MemInst);
          Status != ErrCode::Success) {
        return Status;
      }
      std::vector<unsigned char> MemVec;
      boost::algorithm::unhex(Hex.begin(), Hex.end(),
                              std::back_inserter(MemVec));
      if (ErrCode Status = MemInst->setBytes(MemVec, 0, 0, MemVec.size());
          Status != ErrCode::Success) {
        return Status;
      }
    }
  }
  return ErrCode::Success;
}

/// Resume from JSON. See "include/executor/executor.h"
ErrCode Executor::snapshot(rapidjson::Value &Doc,
                           rapidjson::Document::AllocatorType &Alloc) {
  /// Iterate Global instances.
  unsigned int GlobCnt = StoreMgr.getGlobalInstsCnt();
  if (GlobCnt > 0) {
    rapidjson::Value GlobArr(rapidjson::kArrayType);
    for (unsigned int I = 0; I < GlobCnt; I++) {
      /// Get address and data to string.
      Instance::GlobalInstance *GlobInst = nullptr;
      StoreMgr.getGlobal(I, GlobInst);
      AST::ValVariant Val;
      GlobInst->getValue(Val);
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
  unsigned int MemCnt = StoreMgr.getMemoryInstsCnt();
  if (MemCnt > 0) {
    rapidjson::Value MemArr(rapidjson::kArrayType);
    for (unsigned int I = 0; I < MemCnt; I++) {
      /// Get address and data to string.
      Instance::MemoryInstance *MemInst = nullptr;
      StoreMgr.getMemory(I, MemInst);
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
  return ErrCode::Success;
}

/// Invoke start function. See "include/executor/executor.h".
ErrCode Executor::run() {
  /// Check is the correct state.
  if (Stat != State::ArgsSet)
    return ErrCode::WrongExecutorFlow;

  /// Run start function.
  ErrCode Result = ErrCode::Success;
  if (auto StartAddr = ModInst->getStartAddr()) {
    Result = Engine.runStartFunction(*StartAddr);
    Stat = State::Executed;
  } else {
    StackMgr.reset();
    Result = ErrCode::CallFunctionError;
  }
  return Result;
}

/// Get return values. See "include/executor/executor.h".
ErrCode Executor::getRets(std::vector<Value> &Rets) {
  /// Check is the correct state.
  if (Stat != State::Executed)
    return ErrCode::WrongExecutorFlow;

  /// Push args to stack.
  Rets.clear();
  Rets.resize(StackMgr.size());
  auto Iter = Rets.rbegin();
  while (StackMgr.size() > 0) {
    Value V;
    StackMgr.pop(V);
    *Iter = V;
    Iter++;
  }

  Stat = State::Finished;
  return ErrCode::Success;
}

/// Reset Executor. See "include/executor/executor.h".
ErrCode Executor::reset(bool Force) {
  if (!Force && (Stat != State::Finished && Stat != State::Executed)) {
    return ErrCode::WrongExecutorFlow;
  }
  Mod.reset();
  ModInst = nullptr;
  Engine.reset();
  StackMgr.reset();
  StoreMgr.reset();
  HostFuncMgr.reset();
  Stat = State::Inited;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
