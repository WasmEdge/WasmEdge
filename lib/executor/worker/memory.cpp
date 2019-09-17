#include "executor/instance/memory.h"
#include "ast/instruction.h"
#include "executor/common.h"
#include "executor/instance/module.h"
#include "executor/worker.h"
#include "executor/worker/util.h"
#include "support/casting.h"

#include <cstdint>
#include <type_traits>

namespace SSVM {
namespace Executor {

/// Helper function of getting memory instance by index.
ErrCode getMemInstByIdx(StoreManager &Store, FrameEntry &CurrFrame,
                        unsigned int Idx, Instance::MemoryInstance *&MemInst) {
  ErrCode Status = ErrCode::Success;
  Instance::ModuleInstance *ModInst = nullptr;
  unsigned int MemoryAddr = 0;
  unsigned int ModuleAddr = CurrFrame.getModuleAddr();
  if ((Status = Store.getModule(ModuleAddr, ModInst)) != ErrCode::Success) {
    return Status;
  };
  if ((Status = ModInst->getMemAddr(Idx, MemoryAddr)) != ErrCode::Success) {
    return Status;
  };
  return Store.getMemory(MemoryAddr, MemInst);
}

template <typename T>
TypeT<T, ErrCode> Worker::runLoadOp(AST::MemoryInstruction *Instr,
                                    unsigned int Width) {
  /// Get Memory Instance
  ErrCode Status = ErrCode::Success;
  Instance::MemoryInstance *MemoryInst = nullptr;
  if ((Status = getMemInstByIdx(StoreMgr, *CurrentFrame, 0, MemoryInst)) !=
      ErrCode::Success) {
    return Status;
  };

  /// Calculate EA
  std::unique_ptr<ValueEntry> Val;
  StackMgr.pop(Val);
  uint32_t EA = retrieveValue<uint32_t>(*Val.get()) + Instr->getOffset();

  /// Value = Mem.Data[EA : N / 8]
  T Value;
  if ((Status = MemoryInst->loadValue(EA, Width / 8, Value)) !=
      ErrCode::Success) {
    return Status;
  }

  /// Push const C to the Stack
  std::unique_ptr<ValueEntry> C;
  if (std::is_floating_point_v<T>) {
    C = std::make_unique<ValueEntry>(Value);
  } else {
    C = std::make_unique<ValueEntry>(
        static_cast<std::make_unsigned_t<T>>(Value));
  }
  return StackMgr.push(C);
}

template <typename T>
TypeB<T, ErrCode> Worker::runStoreOp(AST::MemoryInstruction *Instr,
                                     unsigned int Width) {
  /// Get Memory Instance
  ErrCode Status = ErrCode::Success;
  Instance::MemoryInstance *MemoryInst = nullptr;
  if ((Status = getMemInstByIdx(StoreMgr, *CurrentFrame, 0, MemoryInst)) !=
      ErrCode::Success) {
    return Status;
  };

  /// Pop the value t.const c from the Stack
  std::unique_ptr<ValueEntry> C;
  StackMgr.pop(C);

  /// Calculate EA = i + offset
  std::unique_ptr<ValueEntry> I;
  StackMgr.pop(I);
  uint32_t EA = retrieveValue<uint32_t>(*I.get()) + Instr->getOffset();

  /// Store value to bytes.
  T Value = retrieveValue<T>(*C.get());
  if ((Status = MemoryInst->storeValue(EA, Width / 8, Value)) !=
      ErrCode::Success) {
    return Status;
  };
  return ErrCode::Success;
}

ErrCode Worker::runMemorySizeOp() {
  /// Get Memory Instance
  ErrCode Status = ErrCode::Success;
  Instance::MemoryInstance *MemoryInst = nullptr;
  if ((Status = getMemInstByIdx(StoreMgr, *CurrentFrame, 0, MemoryInst)) !=
      ErrCode::Success) {
    return Status;
  };

  /// Push SZ = page size to stack.
  std::unique_ptr<ValueEntry> SZ =
      std::make_unique<ValueEntry>(MemoryInst->getDataPageSize());
  return StackMgr.push(SZ);
}

ErrCode Worker::runMemoryGrowOp() {
  /// Get Memory Instance
  ErrCode Status = ErrCode::Success;
  Instance::MemoryInstance *MemoryInst = nullptr;
  if ((Status = getMemInstByIdx(StoreMgr, *CurrentFrame, 0, MemoryInst)) !=
      ErrCode::Success) {
    return Status;
  };

  /// Pop N for growing page size.
  std::unique_ptr<ValueEntry> N;
  if ((Status = StackMgr.pop(N)) != ErrCode::Success) {
    return Status;
  }

  /// Grow page and push result.
  if (MemoryInst->growPage(retrieveValue<uint32_t>(*N.get())) !=
      ErrCode::Success) {
    return StackMgr.pushValue(static_cast<uint32_t>(-1));
  }
  return StackMgr.pushValue(MemoryInst->getDataPageSize());
}

} // namespace Executor
} // namespace SSVM
