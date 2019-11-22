#include "vm/hostfunc/ethereum/storagestore.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Executor {

EEIStorageStore::EEIStorageStore(VM::EVMEnvironment &Env) : EEI(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode EEIStorageStore::run(std::vector<Value> &Args, std::vector<Value> &Res,
                             StoreManager &Store,
                             Instance::ModuleInstance *ModInst) {
  /// Arg: pathOffset(u32), valueOffset(u32)
  if (Args.size() != 2) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int PathOffset = retrieveValue<uint32_t>(Args[1]);
  unsigned int ValueOffset = retrieveValue<uint32_t>(Args[0]);

  /// Get Path data by path offset.
  std::vector<unsigned char> Data;
  unsigned int MemoryAddr = 0;
  std::string Path("");
  std::string Value("");
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = MemInst->getBytes(Data, PathOffset, 32)) != ErrCode::Success) {
    return Status;
  }
  Support::convertHexToString(Data, Path, 64);

  /// Get Value data by value offset.
  Data.clear();
  if ((Status = MemInst->getBytes(Data, ValueOffset, 32)) != ErrCode::Success) {
    return Status;
  }
  Support::convertHexToString(Data, Value, 64);

  /// Set Value data to storage.
  Env.getStorage()[Path] = Value;

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM
