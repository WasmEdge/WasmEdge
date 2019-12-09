// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/common.h"
#include "executor/hostfunc.h"
#include "executor/worker/util.h"
#include "onncenv.h"

#include <iostream>
#include <string>

namespace SSVM {
namespace Executor {

class ONNCTimeStart : public HostFunction {
public:
  ONNCTimeStart(VM::ONNCEnvironment &ONNCEnv) : Env(ONNCEnv) {
    appendParamDef(AST::ValType::I32);
  }
  ONNCTimeStart() = delete;
  virtual ~ONNCTimeStart() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst) {
    if (Args.size() != 1) {
      return ErrCode::CallFunctionError;
    }
    ErrCode Status = ErrCode::Success;
    unsigned int KeyPtr = retrieveValue<uint32_t>(Args[0]);

    /// Get memory instance.
    unsigned int MemoryAddr = 0;
    Instance::MemoryInstance *MemInst = nullptr;
    if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
      return Status;
    }
    if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
      return Status;
    }

    const char *Key = MemInst->getPointer<char *>(KeyPtr);
    Env.setStart(std::string(Key));
    return ErrCode::Success;
  }

private:
  VM::ONNCEnvironment &Env;
};

class ONNCTimeStop : public HostFunction {
public:
  ONNCTimeStop(VM::ONNCEnvironment &ONNCEnv) : Env(ONNCEnv) {
    appendParamDef(AST::ValType::I32);
    appendParamDef(AST::ValType::I32);
  }
  ONNCTimeStop() = delete;
  virtual ~ONNCTimeStop() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst) {
    if (Args.size() != 2) {
      return ErrCode::CallFunctionError;
    }
    ErrCode Status = ErrCode::Success;
    unsigned int KeyPtr = retrieveValue<uint32_t>(Args[1]);
    unsigned int MsgPtr = retrieveValue<uint32_t>(Args[0]);

    /// Get memory instance.
    unsigned int MemoryAddr = 0;
    Instance::MemoryInstance *MemInst = nullptr;
    if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
      return Status;
    }
    if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
      return Status;
    }

    const char *Key = MemInst->getPointer<char *>(KeyPtr);
    uint64_t T = Env.setStop(std::string(Key));
    const char *Msg = MemInst->getPointer<char *>(MsgPtr);
    std::cerr << " -- " << std::string(Msg) << " cost " << T << " us"
              << std::endl;
    return ErrCode::Success;
  }

private:
  VM::ONNCEnvironment &Env;
};

class ONNCTimeClear : public HostFunction {
public:
  ONNCTimeClear(VM::ONNCEnvironment &ONNCEnv) : Env(ONNCEnv) {
    appendParamDef(AST::ValType::I32);
  }
  ONNCTimeClear() = delete;
  virtual ~ONNCTimeClear() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst) {
    if (Args.size() != 1) {
      return ErrCode::CallFunctionError;
    }
    ErrCode Status = ErrCode::Success;
    unsigned int KeyPtr = retrieveValue<uint32_t>(Args[0]);

    /// Get memory instance.
    unsigned int MemoryAddr = 0;
    Instance::MemoryInstance *MemInst = nullptr;
    if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
      return Status;
    }
    if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
      return Status;
    }

    const char *Key = MemInst->getPointer<char *>(KeyPtr);
    Env.clear(std::string(Key));
    return ErrCode::Success;
  }

private:
  VM::ONNCEnvironment &Env;
};

} // namespace Executor
} // namespace SSVM
