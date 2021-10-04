// SPDX-License-Identifier: Apache-2.0

#include "interpreter/interpreter.h"

namespace WasmEdge {
namespace Interpreter {

Expect<void>
Interpreter::runMemorySizeOp(Runtime::Instance::MemoryInstance &MemInst) {
  /// Push SZ = page size to stack.
  StackMgr.push(MemInst.getPageSize());
  return {};
}

Expect<void>
Interpreter::runMemoryGrowOp(Runtime::Instance::MemoryInstance &MemInst) {
  /// Pop N for growing page size.
  uint32_t &N = StackMgr.getTop().get<uint32_t>();

  /// Grow page and push result.
  const uint32_t CurrPageSize = static_cast<uint32_t>(MemInst.getPageSize());
  if (MemInst.growPage(N)) {
    N = CurrPageSize;
  } else {
    N = static_cast<uint32_t>(-1);
  }
  return {};
}

Expect<void>
Interpreter::runMemoryInitOp(Runtime::Instance::MemoryInstance &MemInst,
                             Runtime::Instance::DataInstance &DataInst,
                             const AST::Instruction &Instr) {
  /// Pop the length, source, and destination from stack.
  uint32_t Len = StackMgr.pop().get<uint32_t>();
  uint32_t Src = StackMgr.pop().get<uint32_t>();
  uint32_t Dst = StackMgr.pop().get<uint32_t>();

  /// Replace mem[Dst : Dst + Len] with data[Src : Src + Len].
  if (auto Res = MemInst.setBytes(DataInst.getData(), Dst, Src, Len)) {
    return {};
  } else {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
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
                             const AST::Instruction &Instr) {
  /// Pop the length, source, and destination from stack.
  uint32_t Len = StackMgr.pop().get<uint32_t>();
  uint32_t Src = StackMgr.pop().get<uint32_t>();
  uint32_t Dst = StackMgr.pop().get<uint32_t>();

  /// Replace mem[Dst : Dst + Len] with mem[Src : Src + Len].
  if (auto Data = MemInst.getBytes(Src, Len)) {
    if (auto Res = MemInst.setBytes(*Data, Dst, 0, Len)) {
      return {};
    } else {
      spdlog::error(
          ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
      return Unexpect(Res);
    }
  } else {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Data);
  }
}

Expect<void>
Interpreter::runMemoryFillOp(Runtime::Instance::MemoryInstance &MemInst,
                             const AST::Instruction &Instr) {
  /// Pop the length, value, and offset from stack.
  uint32_t Len = StackMgr.pop().get<uint32_t>();
  uint8_t Val = static_cast<uint8_t>(StackMgr.pop().get<uint32_t>());
  uint32_t Off = StackMgr.pop().get<uint32_t>();

  /// Fill data with Val.
  if (auto Res = MemInst.fillBytes(Val, Off, Len)) {
    return {};
  } else {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }
}

} // namespace Interpreter
} // namespace WasmEdge
