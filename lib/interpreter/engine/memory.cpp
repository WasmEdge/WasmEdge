// SPDX-License-Identifier: Apache-2.0
#include "common/log.h"
#include "common/value.h"
#include "interpreter/interpreter.h"

namespace SSVM {
namespace Interpreter {

Expect<void>
Interpreter::runMemorySizeOp(Runtime::Instance::MemoryInstance &MemInst) {
  /// Push SZ = page size to stack.
  StackMgr.push(MemInst.getDataPageSize());
  return {};
}

Expect<void>
Interpreter::runMemoryGrowOp(Runtime::Instance::MemoryInstance &MemInst) {
  /// Pop N for growing page size.
  uint32_t &N = retrieveValue<uint32_t>(StackMgr.getTop());

  /// Grow page and push result.
  const uint32_t CurrPageSize = MemInst.getDataPageSize();
  if (MemInst.growPage(N)) {
    N = CurrPageSize;
  } else {
    N = -1;
  }
  return {};
}

Expect<void>
Interpreter::runMemoryInitOp(Runtime::Instance::MemoryInstance &MemInst,
                             Runtime::Instance::DataInstance &DataInst,
                             const AST::MemoryInstruction &Instr) {
  /// Pop the length, source, and destination from stack.
  uint32_t Len = retrieveValue<uint32_t>(StackMgr.pop());
  uint32_t Src = retrieveValue<uint32_t>(StackMgr.pop());
  uint32_t Dst = retrieveValue<uint32_t>(StackMgr.pop());

  /// Replace mem[Dst : Dst + Len] with data[Src : Src + Len].
  if (auto Res = MemInst.setBytes(DataInst.getData(), Dst, Src, Len)) {
    return {};
  } else {
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(Res);
  }
}

Expect<void>
Interpreter::runDataDropOp(Runtime::Instance::DataInstance &DataInst) {
  /// Clear data instance.
  DataInst.clear();
  return {};
}

Expect<void>
Interpreter::runMemoryCopyOp(Runtime::Instance::MemoryInstance &MemInst,
                             const AST::MemoryInstruction &Instr) {
  /// Pop the length, source, and destination from stack.
  uint32_t Len = retrieveValue<uint32_t>(StackMgr.pop());
  uint32_t Src = retrieveValue<uint32_t>(StackMgr.pop());
  uint32_t Dst = retrieveValue<uint32_t>(StackMgr.pop());

  /// Replace mem[Dst : Dst + Len] with mem[Src : Src + Len].
  if (auto Data = MemInst.getBytes(Src, Len)) {
    if (auto Res = MemInst.setBytes(*Data, Dst, 0, Len)) {
      return {};
    } else {
      LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                             Instr.getOffset());
      return Unexpect(Res);
    }
  } else {
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(Data);
  }
}

Expect<void>
Interpreter::runMemoryFillOp(Runtime::Instance::MemoryInstance &MemInst,
                             const AST::MemoryInstruction &Instr) {
  /// Pop the length, value, and offset from stack.
  uint32_t Len = retrieveValue<uint32_t>(StackMgr.pop());
  uint8_t Val = static_cast<uint8_t>(retrieveValue<uint32_t>(StackMgr.pop()));
  uint32_t Off = retrieveValue<uint32_t>(StackMgr.pop());

  /// Fill data with Val.
  if (auto Res = MemInst.fillBytes(Val, Off, Len)) {
    return {};
  } else {
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(Res);
  }
}

Expect<void> Interpreter::runLoadOp(Runtime::Instance::MemoryInstance &MemInst,
                                    const AST::SIMDMemoryInstruction &Instr,
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
  if (auto Res =
          MemInst.loadValue(retrieveValue<uint128_t>(Val), EA, BitWidth / 8);
      !Res) {
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(Res);
  }
  return {};
}

Expect<void> Interpreter::runStoreOp(Runtime::Instance::MemoryInstance &MemInst,
                                     const AST::SIMDMemoryInstruction &Instr) {
  /// Pop the value t.const c from the Stack
  uint128_t C = retrieveValue<uint128_t>(StackMgr.pop());

  /// Calculate EA
  uint32_t I = retrieveValue<uint32_t>(StackMgr.pop());
  if (I > std::numeric_limits<uint32_t>::max() - Instr.getMemoryOffset()) {
    LOG(ERROR) << ErrCode::MemoryOutOfBounds;
    LOG(ERROR) << ErrInfo::InfoBoundary(
        I + static_cast<uint64_t>(Instr.getMemoryOffset()), 16,
        MemInst.getBoundIdx());
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(ErrCode::MemoryOutOfBounds);
  }
  uint32_t EA = I + Instr.getMemoryOffset();

  /// Store value to bytes.
  if (auto Res = MemInst.storeValue(C, EA, 16); !Res) {
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(Res);
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
