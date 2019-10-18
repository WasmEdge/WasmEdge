#include "vm/hostfunc/ethereum/storageload.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Executor {

EEIStorageLoad::EEIStorageLoad(VM::EVMEnvironment &Env) : EEI(Env) {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode EEIStorageLoad::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                            std::vector<std::unique_ptr<ValueEntry>> &Res,
                            StoreManager &Store,
                            Instance::ModuleInstance *ModInst) {
  /// Arg: pathOffset(u32), valueOffset(u32)
  if (Args.size() != 2) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int PathOffset = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int ValueOffset = retrieveValue<uint32_t>(*Args[0].get());

  /// Get Path data by path offset.
  std::vector<unsigned char> Data;
  unsigned int MemoryAddr = 0;
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

  /// Get Value data in storage by key of path.
  std::string Path("");
  std::string Value(64, '0');
  std::map<std::string, std::string> &Storage = Env.getStorage();
  Support::convertHexToString(Data, Path, 64);
  if (Storage.find(Path) != Storage.end()) {
    Value = Storage[Path];
  }

  /// Set Value data to memory.
  Data.clear();
  Support::convertStringToHex(Value, Data, 64);
  if ((Status = MemInst->setBytes(Data, ValueOffset, 0, 32)) !=
      ErrCode::Success) {
    return Status;
  }

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM