// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runTableGetOp(Runtime::StackManager &StackMgr,
                                     Runtime::Instance::TableInstance &TabInst,
                                     const AST::Instruction &Instr) {
  // Pop Idx from Stack.
  const auto AddrType = TabInst.getTableType().getLimit().getAddrType();
  uint64_t Idx = extractAddr(StackMgr.pop(), AddrType);

  // Get table[Idx] and push to Stack.
  return TabInst.getRefAddr(Idx)
      .map_error([&Instr, &Idx](auto E) {
        spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(),
                                               Instr.getOffset(), {Idx},
                                               {ValTypeFromType<uint32_t>()}));
        return E;
      })
      .and_then([&](auto Ref) -> Expect<void> {
        StackMgr.push(Ref);
        return {};
      });
}

Expect<void> Executor::runTableSetOp(Runtime::StackManager &StackMgr,
                                     Runtime::Instance::TableInstance &TabInst,
                                     const AST::Instruction &Instr) {
  // Pop Ref from Stack.
  RefVariant Ref = StackMgr.pop().get<RefVariant>();

  // Pop Idx from Stack.
  const auto AddrType = TabInst.getTableType().getLimit().getAddrType();
  uint64_t Idx = extractAddr(StackMgr.pop(), AddrType);

  // Set table[Idx] with Ref.
  return TabInst.setRefAddr(Idx, Ref).map_error([&Instr, &Idx](auto E) {
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()}));
    return E;
  });
}

Expect<void>
Executor::runTableInitOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::TableInstance &TabInst,
                         Runtime::Instance::ElementInstance &ElemInst,
                         const AST::Instruction &Instr) {
  // Pop the length, source, and destination from stack.
  // Currently, the length and source offset from element instance is defined as
  // 32-bit.
  uint64_t Len = static_cast<uint64_t>(StackMgr.pop().get<uint32_t>());
  uint64_t Src = static_cast<uint64_t>(StackMgr.pop().get<uint32_t>());
  const auto AddrType = TabInst.getTableType().getLimit().getAddrType();
  uint64_t Dst = extractAddr(StackMgr.pop(), AddrType);

  // Replace tab[Dst : Dst + Len] with elem[Src : Src + Len].
  return TabInst.setRefs(ElemInst.getRefs(), Dst, Src, Len)
      .map_error([&Instr](auto E) {
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      });
}

Expect<void>
Executor::runElemDropOp(Runtime::Instance::ElementInstance &ElemInst) {
  // Clear element instance.
  ElemInst.clear();
  return {};
}

Expect<void>
Executor::runTableCopyOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::TableInstance &TabInstDst,
                         Runtime::Instance::TableInstance &TabInstSrc,
                         const AST::Instruction &Instr) {
  // Pop the length, source, and destination from stack.
  const auto AddrType1 = TabInstSrc.getTableType().getLimit().getAddrType();
  const auto AddrType2 = TabInstDst.getTableType().getLimit().getAddrType();
  uint64_t Len = extractAddr(StackMgr.pop(), std::min(AddrType1, AddrType2));
  uint64_t Src = extractAddr(StackMgr.pop(), AddrType2);
  uint64_t Dst = extractAddr(StackMgr.pop(), AddrType1);

  // Replace tab_dst[Dst : Dst + Len] with tab_src[Src : Src + Len].
  return TabInstSrc.getRefs(0, Src + Len)
      .and_then(
          [&](auto Refs) { return TabInstDst.setRefs(Refs, Dst, Src, Len); })
      .map_error([&Instr](auto E) {
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      });
}

Expect<void>
Executor::runTableGrowOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::TableInstance &TabInst) {
  // Pop N for growing size, Val for init ref value.
  const auto AddrType = TabInst.getTableType().getLimit().getAddrType();
  uint64_t N = extractAddr(StackMgr.pop(), AddrType);
  RefVariant Ref = StackMgr.pop().get<RefVariant>();

  // Grow size and push result.
  const uint64_t CurrSize = TabInst.getSize();
  if (TabInst.growTable(N, Ref)) {
    StackMgr.push(emplaceAddr(CurrSize, AddrType));
  } else {
    StackMgr.push(emplaceAddr(static_cast<uint64_t>(-1), AddrType));
  }
  return {};
}

Expect<void>
Executor::runTableSizeOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::TableInstance &TabInst) {
  // Push SZ = size to stack.
  const auto AddrType = TabInst.getTableType().getLimit().getAddrType();
  StackMgr.push(emplaceAddr(TabInst.getSize(), AddrType));
  return {};
}

Expect<void> Executor::runTableFillOp(Runtime::StackManager &StackMgr,
                                      Runtime::Instance::TableInstance &TabInst,
                                      const AST::Instruction &Instr) {
  // Pop the length, ref_value, and offset from stack.
  const auto AddrType = TabInst.getTableType().getLimit().getAddrType();
  uint64_t Len = extractAddr(StackMgr.pop(), AddrType);
  RefVariant Val = StackMgr.pop().get<RefVariant>();
  uint64_t Off = extractAddr(StackMgr.pop(), AddrType);

  // Fill refs with ref_value.
  return TabInst.fillRefs(Val, Off, Len).map_error([&Instr](auto E) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return E;
  });
}

} // namespace Executor
} // namespace WasmEdge
