// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include <cstring>

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
  uint64_t N = extractAddr(StackMgr.pop<ValVariant>(), AddrType);

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
  auto [RawLen, RawSrc, RawDst] =
      StackMgr.pops<uint32_t, uint32_t, ValVariant>();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Len = static_cast<uint64_t>(RawLen);
  uint64_t Src = static_cast<uint64_t>(RawSrc);
  uint64_t Dst = extractAddr(RawDst, AddrType);

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
  auto [RawLen, RawSrc, RawDst] =
      StackMgr.pops<ValVariant, ValVariant, ValVariant>();
  const auto AddrType1 = MemInstSrc.getMemoryType().getLimit().getAddrType();
  const auto AddrType2 = MemInstDst.getMemoryType().getLimit().getAddrType();
  uint64_t Len = extractAddr(RawLen, std::min(AddrType1, AddrType2));
  uint64_t Src = extractAddr(RawSrc, AddrType2);
  uint64_t Dst = extractAddr(RawDst, AddrType1);

  // Replace mem[Dst : Dst + Len] with mem[Src : Src + Len].
  // When source and destination are the same memory instance, overlapping
  // regions require memmove semantics per the Wasm spec.
  if (&MemInstSrc == &MemInstDst) {
    // Same memory: validate bounds, then use memmove for overlap safety.
    EXPECTED_TRY(MemInstSrc.getBytes(Src, Len).map_error([&Instr](auto E) {
      spdlog::error(
          ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
      return E;
    }));
    EXPECTED_TRY(MemInstDst.getBytes(Dst, Len).map_error([&Instr](auto E) {
      spdlog::error(
          ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
      return E;
    }));
    if (likely(Len > 0)) {
      std::memmove(MemInstDst.getDataPtr() + Dst, MemInstSrc.getDataPtr() + Src,
                   Len);
    }
    return {};
  } else {
    // Different memories: no overlap possible, use the existing path.
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
}

Expect<void>
Executor::runMemoryFillOp(Runtime::StackManager &StackMgr,
                          Runtime::Instance::MemoryInstance &MemInst,
                          const AST::Instruction &Instr) {
  // Pop the length, value, and offset from stack.
  auto [RawLen, RawVal, RawOff] =
      StackMgr.pops<ValVariant, uint32_t, ValVariant>();
  const auto AddrType = MemInst.getMemoryType().getLimit().getAddrType();
  uint64_t Len = extractAddr(RawLen, AddrType);
  uint8_t Val = static_cast<uint8_t>(RawVal);
  uint64_t Off = extractAddr(RawOff, AddrType);

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
