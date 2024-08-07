// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runTableGetOp(Runtime::StackManager &StackMgr,
                                     Runtime::Instance::TableInstance &TabInst,
                                     const AST::Instruction &Instr) {
  // Pop Idx from Stack.
  uint32_t Idx = StackMgr.pop().get<uint32_t>();

  // Get table[Idx] and push to Stack.
  if (auto Res = TabInst.getRefAddr(Idx)) {
    StackMgr.push(*Res);
  } else {
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()}));
    return Unexpect(Res);
  }
  return {};
}

Expect<void> Executor::runTableSetOp(Runtime::StackManager &StackMgr,
                                     Runtime::Instance::TableInstance &TabInst,
                                     const AST::Instruction &Instr) {
  // Pop Ref from Stack.
  RefVariant Ref = StackMgr.pop().get<RefVariant>();

  // Pop Idx from Stack.
  uint32_t Idx = StackMgr.pop().get<uint32_t>();

  // Set table[Idx] with Ref.
  if (auto Res = TabInst.setRefAddr(Idx, Ref); !Res) {
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()}));
    return Unexpect(Res);
  }
  return {};
}

Expect<void>
Executor::runTableInitOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::TableInstance &TabInst,
                         Runtime::Instance::ElementInstance &ElemInst,
                         const AST::Instruction &Instr) {
  // Pop the length, source, and destination from stack.
  uint32_t Len = StackMgr.pop().get<uint32_t>();
  uint32_t Src = StackMgr.pop().get<uint32_t>();
  uint32_t Dst = StackMgr.pop().get<uint32_t>();

  // Replace tab[Dst : Dst + Len] with elem[Src : Src + Len].
  if (auto Res = TabInst.setRefs(ElemInst.getRefs(), Dst, Src, Len)) {
    return {};
  } else {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }
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
  uint32_t Len = StackMgr.pop().get<uint32_t>();
  uint32_t Src = StackMgr.pop().get<uint32_t>();
  uint32_t Dst = StackMgr.pop().get<uint32_t>();

  // Replace tab_dst[Dst : Dst + Len] with tab_src[Src : Src + Len].
  if (auto Refs = TabInstSrc.getRefs(0, Src + Len)) {
    if (auto Res = TabInstDst.setRefs(*Refs, Dst, Src, Len)) {
      return {};
    } else {
      spdlog::error(
          ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
      return Unexpect(Res);
    }
  } else {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Refs);
  }
}

Expect<void>
Executor::runTableGrowOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::TableInstance &TabInst) {
  // Pop N for growing size, Val for init ref value.
  uint32_t N = StackMgr.pop().get<uint32_t>();
  ValVariant &Val = StackMgr.getTop();

  // Grow size and push result.
  const uint32_t CurrSize = TabInst.getSize();

  if (TabInst.growTable(N, Val.get<RefVariant>())) {
    Val.emplace<uint32_t>(CurrSize);
  } else {
    Val.emplace<int32_t>(INT32_C(-1));
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
  uint32_t Len = StackMgr.pop().get<uint32_t>();
  RefVariant Val = StackMgr.pop().get<RefVariant>();
  uint32_t Off = StackMgr.pop().get<uint32_t>();

  // Fill refs with ref_value.
  if (auto Res = TabInst.fillRefs(Val, Off, Len)) {
    return {};
  } else {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }
}

} // namespace Executor
} // namespace WasmEdge
