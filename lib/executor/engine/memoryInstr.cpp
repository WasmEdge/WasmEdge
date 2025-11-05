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
  uint32_t N = StackMgr.peekTop<uint32_t>();

  // Grow page and push result.
  const uint32_t CurrPageSize = static_cast<uint32_t>(MemInst.getPageSize());
  if (MemInst.growPage(N)) {
    StackMgr.emplaceTop(CurrPageSize);
  } else {
    StackMgr.emplaceTop(UINT32_C(0xffffffff));
  }
  return {};
}

Expect<void> Executor::runMemoryInitOp(
    Runtime::StackManager &StackMgr, Runtime::Instance::MemoryInstance &MemInst,
    Runtime::Instance::DataInstance &DataInst, const AST::Instruction &Instr) {
  // Pop the length, source, and destination from stack.
  auto [Len, Src, Dst] = StackMgr.pops<uint32_t, uint32_t, uint32_t>();

  // Replace mem[Dst : Dst + Len] with data[Src : Src + Len].
  return MemInst.setBytes(DataInst.getData(), Dst, Src, Len)
      .map_error([&Instr](auto E) {
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      });
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
  auto [Len, Src, Dst] = StackMgr.pops<uint32_t, uint32_t, uint32_t>();

  // Replace mem[Dst : Dst + Len] with mem[Src : Src + Len].
  EXPECTED_TRY(auto Data,
               MemInstSrc.getBytes(Src, Len).map_error([&Instr](auto E) {
                 spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(),
                                                        Instr.getOffset()));
                 return E;
               }));
  return MemInstDst.setBytes(Data, Dst, 0, Len).map_error([&Instr](auto E) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return E;
  });
}

Expect<void>
Executor::runMemoryFillOp(Runtime::StackManager &StackMgr,
                          Runtime::Instance::MemoryInstance &MemInst,
                          const AST::Instruction &Instr) {
  // Pop the length, value, and offset from stack.
  auto [Len, Val, Off] = StackMgr.pops<uint32_t, uint32_t, uint32_t>();

  // Fill data with Val.
  return MemInst.fillBytes(static_cast<uint8_t>(Val), Off, Len)
      .map_error([&Instr](auto E) {
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      });
}

} // namespace Executor
} // namespace WasmEdge
