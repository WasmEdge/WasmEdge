// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void>
Executor::runMemorySizeOp(Runtime::StackManager &StackMgr,
                          Runtime::Instance::MemoryInstance &MemInst) {
  // Push SZ = page size to stack.
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  StackMgr.push(emplaceAddr(MemInst.getPageSize(), AddrType));
  return {};
}

Expect<void>
Executor::runMemoryGrowOp(Runtime::StackManager &StackMgr,
                          Runtime::Instance::MemoryInstance &MemInst) {
  // Pop N for growing page size.
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t N = extractAddr(StackMgr.pop(), AddrType);

  // Grow page and push result.
  const uint64_t CurrPageSize = MemInst.getPageSize();
  if (MemInst.growPage(N)) {
    StackMgr.push(emplaceAddr(CurrPageSize, AddrType));
  } else {
    StackMgr.push(emplaceAddr(static_cast<uint64_t>(-1), AddrType));
  }
  return {};
}

Expect<void> Executor::runMemoryInitOp(
    Runtime::StackManager &StackMgr, Runtime::Instance::MemoryInstance &MemInst,
    Runtime::Instance::DataInstance &DataInst, const AST::Instruction &Instr) {
  // Pop the length, source, and destination from stack.
  // Currently, the length and source offset from data instance is defined as
  // 32-bit.
  uint64_t Len = static_cast<uint64_t>(StackMgr.pop().get<uint32_t>());
  uint64_t Src = static_cast<uint64_t>(StackMgr.pop().get<uint32_t>());
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Dst = extractAddr(StackMgr.pop(), AddrType);

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
  const auto AddrType1 = MemInstSrc.getMemoryType().getLimit().getAddrType();
  const auto AddrType2 = MemInstDst.getMemoryType().getLimit().getAddrType();
  uint64_t Len = extractAddr(StackMgr.pop(), std::min(AddrType1, AddrType2));
  uint64_t Src = extractAddr(StackMgr.pop(), AddrType2);
  uint64_t Dst = extractAddr(StackMgr.pop(), AddrType1);

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
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Len = extractAddr(StackMgr.pop(), AddrType);
  uint8_t Val = static_cast<uint8_t>(StackMgr.pop().get<uint32_t>());
  uint64_t Off = extractAddr(StackMgr.pop(), AddrType);

  // Fill data with Val.
  return MemInst.fillBytes(Val, Off, Len).map_error([&Instr](auto E) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return E;
  });
}

} // namespace Executor
} // namespace WasmEdge
