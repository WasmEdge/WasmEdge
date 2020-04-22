// SPDX-License-Identifier: Apache-2.0
#include "common/ast/instruction.h"
#include "common/value.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/memory.h"

#include <cstdint>

namespace SSVM {
namespace Interpreter {

template <typename T>
TypeT<T> Interpreter::runLoadOp(Runtime::Instance::MemoryInstance &MemInst,
                                const AST::MemoryInstruction &Instr,
                                const uint32_t BitWidth) {
  /// Calculate EA
  ValVariant &Val = StackMgr.getTop();
  if (retrieveValue<uint32_t>(Val) >
      std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    return Unexpect(ErrCode::AccessForbidMemory);
  }
  uint32_t EA = retrieveValue<uint32_t>(Val) + Instr.getMemoryOffset();

  /// Value = Mem.Data[EA : N / 8]
  return MemInst.loadValue(retrieveValue<T>(Val), EA, BitWidth / 8);
}

template <typename T>
TypeB<T> Interpreter::runStoreOp(Runtime::Instance::MemoryInstance &MemInst,
                                 const AST::MemoryInstruction &Instr,
                                 const uint32_t BitWidth) {
  /// Pop the value t.const c from the Stack
  ValVariant C = StackMgr.pop();

  /// Calculate EA = i + offset
  ValVariant I = StackMgr.pop();
  if (retrieveValue<uint32_t>(I) >
      std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    return Unexpect(ErrCode::AccessForbidMemory);
  }
  uint32_t EA = retrieveValue<uint32_t>(I) + Instr.getMemoryOffset();

  /// Store value to bytes.
  return MemInst.storeValue(retrieveValue<T>(C), EA, BitWidth / 8);
}

} // namespace Interpreter
} // namespace SSVM
