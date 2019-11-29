// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/revert.h"
#include "executor/common.h"
#include "executor/worker/util.h"

namespace SSVM {
namespace Executor {

EEIRevert::EEIRevert(VM::EVMEnvironment &Env) : EEI(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode EEIRevert::run(std::vector<Value> &Args, std::vector<Value> &Res,
                       StoreManager &Store, Instance::ModuleInstance *ModInst) {
  /// Arg: dataOffset(u32), dataLength(u32)
  if (Args.size() != 2) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int DataOffset = retrieveValue<uint32_t>(Args[1]);
  unsigned int DataLength = retrieveValue<uint32_t>(Args[0]);
  Env.getReturnData().clear();

  if (DataLength > 0) {
    unsigned int MemoryAddr = 0;
    Instance::MemoryInstance *MemInst = nullptr;
    if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
      return Status;
    }
    if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
      return Status;
    }
    if ((Status = MemInst->getBytes(Env.getReturnData(), DataOffset,
                                    DataLength)) != ErrCode::Success) {
      return Status;
    }
  }

  /// Return: void
  return ErrCode::Revert;
}

} // namespace Executor
} // namespace SSVM
