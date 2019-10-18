#include "vm/hostfunc/ethereum/returndatacopy.h"
#include "executor/common.h"
#include "executor/worker/util.h"

namespace SSVM {
namespace Executor {

EEIReturnDataCopy::EEIReturnDataCopy(VM::EVMEnvironment &Env) : EEI(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode EEIReturnDataCopy::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                               std::vector<std::unique_ptr<ValueEntry>> &Res,
                               StoreManager &Store,
                               Instance::ModuleInstance *ModInst) {
  /// Arg: resultOffset(u32), dataOffset(u32), length(u32)
  if (Args.size() != 3) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int ResOffset = retrieveValue<uint32_t>(*Args[2].get());
  unsigned int DataOffset = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int Length = retrieveValue<uint32_t>(*Args[0].get());

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