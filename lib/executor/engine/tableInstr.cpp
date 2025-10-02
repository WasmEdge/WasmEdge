// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runTableGetOp(Runtime::StackManager &StackMgr,
                                     Runtime::Instance::TableInstance &TabInst,
                                     const AST::Instruction &Instr) {
  // Pop Idx from Stack.
  uint32_t Idx = StackMgr.peekTop<uint32_t>();

  // Get table[Idx] and push to Stack.
  return TabInst.getRefAddr(Idx)
      .map_error([&Instr, &Idx](auto E) {
        spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(),
                                               Instr.getOffset(), {Idx},
                                               {ValTypeFromType<uint32_t>()}));
        return E;
      })
      .and_then([&](RefVariant Ref) -> Expect<void> {
        StackMgr.emplaceTop(std::move(Ref));
        return {};
      });
}

Expect<void> Executor::runTableSetOp(Runtime::StackManager &StackMgr,
                                     Runtime::Instance::TableInstance &TabInst,
                                     const AST::Instruction &Instr) {
  // Pop Ref and Idx from Stack.
  auto [Ref, Idx] = StackMgr.pops<RefVariant, uint32_t>();

  // Set table[Idx] with Ref.
  return TabInst.setRefAddr(Idx, Ref).map_error([&Instr, Idx = Idx](auto E) {
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
  auto [Len, Src, Dst] = StackMgr.pops<uint32_t, uint32_t, uint32_t>();

  // Replace tab[Dst : Dst + Len] with elem[Src : Src + Len].
  return ElemInst.getRefs(Src, Len)
      .and_then([&, Dst = Dst](Span<const RefVariant> Refs) {
        return TabInst.setRefs(Refs, Dst);
      })
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
  auto [Len, Src, Dst] = StackMgr.pops<uint32_t, uint32_t, uint32_t>();

  // Replace tab_dst[Dst : Dst + Len] with tab_src[Src : Src + Len].
  return TabInstSrc.getRefs(Src, Len)
      .and_then([&, Dst = Dst](Span<const RefVariant> Refs) {
        return TabInstDst.setRefs(Refs, Dst);
      })
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
  auto [N, RefVal] = StackMgr.pops<uint32_t, RefVariant>();

  // Grow size and push result.
  const uint32_t CurrSize = TabInst.getSize();

  if (TabInst.growTable(N, RefVal)) {
    StackMgr.push(CurrSize);
  } else {
    StackMgr.push(INT32_C(-1));
  }
  return {};
}

Expect<void>
Executor::runTableSizeOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::TableInstance &TabInst) {
  // Push SZ = size to stack.
  StackMgr.push(TabInst.getSize());
  return {};
}

Expect<void> Executor::runTableFillOp(Runtime::StackManager &StackMgr,
                                      Runtime::Instance::TableInstance &TabInst,
                                      const AST::Instruction &Instr) {
  // Pop the length, ref_value, and offset from stack.
  auto [Len, Val, Off] = StackMgr.pops<uint32_t, RefVariant, uint32_t>();

  // Fill refs with ref_value.
  return TabInst.fillRefs(Val, Off, Len).map_error([&Instr](auto E) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return E;
  });
}

} // namespace Executor
} // namespace WasmEdge
