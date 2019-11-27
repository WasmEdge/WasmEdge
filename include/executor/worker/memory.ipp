#include "ast/instruction.h"
#include "executor/common.h"
#include "executor/instance/memory.h"
#include "executor/worker.h"
#include "executor/worker/util.h"
#include "support/casting.h"

#include <cstdint>
#include <type_traits>

namespace SSVM {
namespace Executor {

template <typename T>
TypeT<T, ErrCode> Worker::runLoadOp(AST::MemoryInstruction &Instr,
                                    unsigned int BitWidth) {
  /// Get Memory Instance
  ErrCode Status = ErrCode::Success;
  Instance::MemoryInstance *MemoryInst = nullptr;
  if ((Status = getMemInstByIdx(0, MemoryInst)) != ErrCode::Success) {
    return Status;
  };

  /// Calculate EA
  Value &Val = StackMgr.getTop();
  uint32_t EA = retrieveValue<uint32_t>(Val) + Instr.getMemoryOffset();

  /// Value = Mem.Data[EA : N / 8]
  return MemoryInst->loadValue(retrieveValue<T>(Val), EA, BitWidth / 8);
}

template <typename T>
TypeB<T, ErrCode> Worker::runStoreOp(AST::MemoryInstruction &Instr,
                                     unsigned int BitWidth) {
  /// Get Memory Instance
  ErrCode Status = ErrCode::Success;
  Instance::MemoryInstance *MemoryInst = nullptr;
  if ((Status = getMemInstByIdx(0, MemoryInst)) != ErrCode::Success) {
    return Status;
  };

  /// Pop the value t.const c from the Stack
  Value C;
  StackMgr.pop(C);

  /// Calculate EA = i + offset
  Value I;
  StackMgr.pop(I);
  uint32_t EA = retrieveValue<uint32_t>(I) + Instr.getMemoryOffset();

  /// Store value to bytes.
  T Value = retrieveValue<T>(C);
  if ((Status = MemoryInst->storeValue(Value, EA, BitWidth / 8)) !=
      ErrCode::Success) {
    return Status;
  };
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
