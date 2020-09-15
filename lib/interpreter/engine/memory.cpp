// SPDX-License-Identifier: Apache-2.0
#include "common/value.h"
#include "interpreter/interpreter.h"
#include "support/log.h"

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

} // namespace Interpreter
} // namespace SSVM
