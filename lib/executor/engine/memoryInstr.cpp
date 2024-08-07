// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void>
Executor::runMemorySizeOp(Runtime::StackManager &StackMgr,
                          Runtime::Instance::MemoryInstance &MemInst) {
  // Push SZ = page size to stack.
  StackMgr.push(MemInst.getPageSize());
  return {};
}

Expect<void>
Executor::runMemoryGrowOp(Runtime::StackManager &StackMgr,
                          Runtime::Instance::MemoryInstance &MemInst) {
  // Pop N for growing page size.
  uint32_t &N = StackMgr.getTop().get<uint32_t>();

  // Grow page and push result.
  const uint32_t CurrPageSize = static_cast<uint32_t>(MemInst.getPageSize());
  if (MemInst.growPage(N)) {
    N = CurrPageSize;
  } else {
    N = static_cast<uint32_t>(-1);
  }
  return {};
}

Expect<void> Executor::runMemoryInitOp(
    Runtime::StackManager &StackMgr, Runtime::Instance::MemoryInstance &MemInst,
    Runtime::Instance::DataInstance &DataInst, const AST::Instruction &Instr) {
  // Pop the length, source, and destination from stack.
  uint32_t Len = StackMgr.pop().get<uint32_t>();
  uint32_t Src = StackMgr.pop().get<uint32_t>();
  uint32_t Dst = StackMgr.pop().get<uint32_t>();

  // Replace mem[Dst : Dst + Len] with data[Src : Src + Len].
  if (auto Res = MemInst.setBytes(DataInst.getData(), Dst, Src, Len)) {
    return {};
  } else {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }
}

Expect<void>
Executor::runDataDropOp(Runtime::Instance::DataInstance &DataInst) {
  // Clear data instance.
  DataInst.clear();
  return {};
}

Expect<void>
Executor::runMemoryCopyOp(Runtime::StackManager &StackMgr,
                          Runtime::Instance::MemoryInstance &MemInstDst,
                          Runtime::Instance::MemoryInstance &MemInstSrc,
                          const AST::Instruction &Instr) {
  // Pop the length, source, and destination from stack.
  uint32_t Len = StackMgr.pop().get<uint32_t>();
  uint32_t Src = StackMgr.pop().get<uint32_t>();
  uint32_t Dst = StackMgr.pop().get<uint32_t>();

  // Replace mem[Dst : Dst + Len] with mem[Src : Src + Len].
  if (auto Data = MemInstSrc.getBytes(Src, Len)) {
    if (auto Res = MemInstDst.setBytes(*Data, Dst, 0, Len)) {
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
Executor::runMemoryFillOp(Runtime::StackManager &StackMgr,
                          Runtime::Instance::MemoryInstance &MemInst,
                          const AST::Instruction &Instr) {
  // Pop the length, value, and offset from stack.
  uint32_t Len = StackMgr.pop().get<uint32_t>();
  uint8_t Val = static_cast<uint8_t>(StackMgr.pop().get<uint32_t>());
  uint32_t Off = StackMgr.pop().get<uint32_t>();

  // Fill data with Val.
  if (auto Res = MemInst.fillBytes(Val, Off, Len)) {
    return {};
  } else {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }
}

} // namespace Executor
} // namespace WasmEdge
