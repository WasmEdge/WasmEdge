#include "vm/hostfunc/ethereum/callstatic.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Executor {

ErrCode EEICallStatic::run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                           std::vector<std::unique_ptr<ValueEntry>> &Res,
                           StoreManager &Store,
                           Instance::ModuleInstance *ModInst) {
  /// Arg: gas(u32), addressOffset(u32), dataOffset(u32), dataLength(u32)
  if (Args.size() != 1) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int Gas = retrieveValue<uint32_t>(*Args[3].get());
  unsigned int AddressOffset = retrieveValue<uint32_t>(*Args[2].get());
  unsigned int DataOffset = retrieveValue<uint32_t>(*Args[1].get());
  unsigned int DataLength = retrieveValue<uint32_t>(*Args[0].get());

  std::vector<unsigned char> Address;
  std::vector<unsigned char> Data;
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = MemInst->getBytes(Address, AddressOffset, 20)) !=
      ErrCode::Success) {
    return Status;
  }
  if ((Status = MemInst->getBytes(Data, DataOffset, DataLength)) !=
      ErrCode::Success) {
    return Status;
  }
  /// TODO: run VM by address and set result to Env.ResultValue.

  /// Return: result(u32)
  Res.push_back(std::make_unique<ValueEntry>(0U));
  return Status;
}

} // namespace Executor
} // namespace SSVM