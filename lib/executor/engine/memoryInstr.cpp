// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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
  switch (MemInst.getMemoryType().getIdxType()) {
  case AST::MemoryType::IndexType::I64: {
    uint64_t &N = StackMgr.getTop().get<uint64_t>();
    const uint64_t CurrPageSize = MemInst.getPageSize();
    if (MemInst.growPage(N)) {
      N = CurrPageSize;
    } else {
      N = static_cast<uint64_t>(-1);
    }
    break;
  }
  case AST::MemoryType::IndexType::I32:
  default: {
    uint32_t &N = StackMgr.getTop().get<uint32_t>();
    // Grow page and push result.
    const uint32_t CurrPageSize = static_cast<uint32_t>(MemInst.getPageSize());
    if (MemInst.growPage(N)) {
      N = CurrPageSize;
    } else {
      N = static_cast<uint32_t>(-1);
    }
    break;
  }
  }

  return {};
}

Expect<void> Executor::runMemoryInitOp(
    Runtime::StackManager &StackMgr, Runtime::Instance::MemoryInstance &MemInst,
    Runtime::Instance::DataInstance &DataInst, const AST::Instruction &Instr) {
  // Pop the length, source, and destination from stack.
  auto Len = StackMgr.popIndexType(MemInst.getMemoryType().getIdxType());
  auto Src = StackMgr.popIndexType(MemInst.getMemoryType().getIdxType());
  auto Dst = StackMgr.popIndexType(MemInst.getMemoryType().getIdxType());

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
  auto Len = StackMgr.popIndexType(MemInstSrc.getMemoryType().getIdxType());
  auto Src = StackMgr.popIndexType(MemInstSrc.getMemoryType().getIdxType());
  auto Dst = StackMgr.popIndexType(MemInstDst.getMemoryType().getIdxType());

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
  auto Len = StackMgr.popIndexType(MemInst.getMemoryType().getIdxType());
  uint8_t Val = static_cast<uint8_t>(StackMgr.pop().get<uint32_t>());
  auto Off = StackMgr.popIndexType(MemInst.getMemoryType().getIdxType());

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
