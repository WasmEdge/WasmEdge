// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/returndatacopy.h"
#include "executor/common.h"
#include "executor/worker/util.h"

namespace SSVM {
namespace Executor {

EEIReturnDataCopy::EEIReturnDataCopy(VM::EVMEnvironment &Env, uint64_t Cost)
    : EEI(Env, Cost) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode EEIReturnDataCopy::run(VM::EnvironmentManager &EnvMgr,
                               std::vector<Value> &Args,
                               std::vector<Value> &Res, StoreManager &Store,
                               Instance::ModuleInstance *ModInst) {
  /// Arg: resultOffset(u32), dataOffset(u32), length(u32)
  if (Args.size() != 3) {
    return ErrCode::CallFunctionError;
  }
  /// Add cost.
  if (!EnvMgr.addCost(Cost)) {
    return ErrCode::Revert;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int ResOffset = retrieveValue<uint32_t>(Args[2]);
  unsigned int DataOffset = retrieveValue<uint32_t>(Args[1]);
  unsigned int Length = retrieveValue<uint32_t>(Args[0]);

  if (Length > 0) {
    unsigned int MemoryAddr = 0;
    Instance::MemoryInstance *MemInst = nullptr;
    if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
      return Status;
    }
    if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
      return Status;
    }
    if ((Status = MemInst->setBytes(Env.getReturnData(), ResOffset, DataOffset,
                                    Length)) != ErrCode::Success) {
      return Status;
    }
  }

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
