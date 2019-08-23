#include "ast/instruction.h"
#include "executor/common.h"
#include "executor/instance/memory.h"
#include "executor/instance/module.h"
#include "executor/worker.h"
#include "executor/worker_util.h"
#include "support/casting.h"

#include <cstdint>
#include <type_traits>

namespace SSVM {
namespace Executor {

template <typename T>
ErrCode Worker::runLoadOp(AST::MemoryInstruction *InstrPtr) {
  StackMgr.getCurrentFrame(CurrentFrame);

  /// Get Memory Instance
  unsigned int ModuleAddr = CurrentFrame->getModuleAddr();
  Instance::ModuleInstance *ModuleInst = nullptr;
  StoreMgr.getModule(ModuleAddr, ModuleInst);
  unsigned int MemAddr;
  ModuleInst->getMemAddr(0, MemAddr);
  Instance::MemoryInstance *MemoryInst = nullptr;
  StoreMgr.getMemory(MemAddr, MemoryInst);

  /// Calculate EA
  std::unique_ptr<ValueEntry> Val;
  StackMgr.pop(Val);

  int32_t EA = retrieveValue<int32_t>(*Val.get()) + InstrPtr->getOffset();
  /// FIXME: The following codes do not handle the `t.loadN_sx`.

  /// Get the bit width from the instruction
  AST::Instruction::OpCode Opcode = InstrPtr->getOpCode();
  int N = 0;
  if (std::is_same<T, int32_t>::value) {
    N = 32;
  } else if (std::is_same<T, int64_t>::value) {
    N = 64;
  } else {
    return ErrCode::Unimplemented;
  }

  /// Make sure the EA + N/8 is NOT larger than length of memory.data.
  if (EA + N / 8 > MemoryInst->getDataLength()) {
    return ErrCode::AccessForbidMemory;
  }

  /// Bytes = Mem.Data[EA:N/8]
  std::unique_ptr<std::vector<unsigned char>> BytesPtr = nullptr;
  MemoryInst->getBytes(BytesPtr, EA, N / 8);
  std::unique_ptr<ValueEntry> C = nullptr;
  C = std::make_unique<ValueEntry>(Support::bytesToInt<T>(*BytesPtr.get()));

  /// Push const C to the Stack
  StackMgr.push(C);
  return ErrCode::Success;
}

template <typename T>
ErrCode Worker::runStoreOp(AST::MemoryInstruction *InstrPtr) {
  StackMgr.getCurrentFrame(CurrentFrame);

  unsigned int ModuleAddr = CurrentFrame->getModuleAddr();
  Instance::ModuleInstance *ModuleInst = nullptr;
  StoreMgr.getModule(ModuleAddr, ModuleInst);
  unsigned int MemAddr;
  ModuleInst->getMemAddr(0, MemAddr);
  Instance::MemoryInstance *MemoryInst = nullptr;
  StoreMgr.getMemory(MemAddr, MemoryInst);

  /// Pop the value t.const c from the Stack
  std::unique_ptr<ValueEntry> C;
  StackMgr.pop(C);
  /// Pop the i32.const i from the Stack
  std::unique_ptr<ValueEntry> I;
  StackMgr.pop(I);
  /// EA = i + offset
  int32_t EA = retrieveValue<int32_t>(*I.get()) + InstrPtr->getOffset();
  /// N = bits(t)
  int N = 0;
  if (std::is_same<T, int64_t>::value) {
    N = 64;
  } else if (std::is_same<T, int32_t>::value) {
    N = 32;
  } else {
    return ErrCode::Unimplemented;
  }
  /// EA + N/8 <= Memory.Data.Length
  if (EA + N / 8 > MemoryInst->getDataLength()) {
    return ErrCode::AccessForbidMemory;
  }
  /// b* = toBytes(c)
  std::vector<unsigned char> Bytes =
      Support::intToBytes<T>(retrieveValue<T>(*C.get()));
  /// Replace the bytes.mem.data[EA:N/8] with b*
  MemoryInst->setBytes(Bytes, EA, N / 8);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
