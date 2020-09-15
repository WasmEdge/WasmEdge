// SPDX-License-Identifier: Apache-2.0
#include "common/ast/instruction.h"
#include "common/value.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/memory.h"
#include "support/log.h"

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
    LOG(ERROR) << ErrCode::MemoryOutOfBounds;
    LOG(ERROR) << ErrInfo::InfoBoundary(
        retrieveValue<uint32_t>(Val) +
            static_cast<uint64_t>(Instr.getMemoryOffset()),
        BitWidth / 8, MemInst.getBoundIdx());
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  uint32_t EA = retrieveValue<uint32_t>(Val) + Instr.getMemoryOffset();

  /// Value = Mem.Data[EA : N / 8]
  if (auto Res = MemInst.loadValue(retrieveValue<T>(Val), EA, BitWidth / 8);
      !Res) {
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(Res);
  }
  return {};
}

template <typename T>
TypeB<T> Interpreter::runStoreOp(Runtime::Instance::MemoryInstance &MemInst,
                                 const AST::MemoryInstruction &Instr,
                                 const uint32_t BitWidth) {
  /// Pop the value t.const c from the Stack
  T C = retrieveValue<T>(StackMgr.pop());

  /// Calculate EA = i + offset
  uint32_t I = retrieveValue<uint32_t>(StackMgr.pop());
  if (I > std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    LOG(ERROR) << ErrCode::MemoryOutOfBounds;
    LOG(ERROR) << ErrInfo::InfoBoundary(
        I + static_cast<uint64_t>(Instr.getMemoryOffset()), BitWidth / 8,
        MemInst.getBoundIdx());
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  uint32_t EA = I + Instr.getMemoryOffset();

  /// Store value to bytes.
  if (auto Res = MemInst.storeValue(C, EA, BitWidth / 8); !Res) {
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(Res);
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
