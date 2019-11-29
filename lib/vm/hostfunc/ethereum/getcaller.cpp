// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getcaller.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Executor {

EEIGetCaller::EEIGetCaller(VM::EVMEnvironment &Env) : EEI(Env) {
  appendParamDef(AST::ValType::I32);
}

ErrCode EEIGetCaller::run(std::vector<Value> &Args, std::vector<Value> &Res,
                          StoreManager &Store,
                          Instance::ModuleInstance *ModInst) {
  /// Arg: resultOffset(u32)
  if (Args.size() != 1) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int ResOffset = retrieveValue<uint32_t>(Args[0]);

  std::vector<unsigned char> Data;
  Support::convertStringToHex(Env.getCaller(), Data, 40);
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = MemInst->setBytes(Data, ResOffset, 0, 20)) !=
      ErrCode::Success) {
    return Status;
  }

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
