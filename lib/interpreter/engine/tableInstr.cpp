// SPDX-License-Identifier: Apache-2.0

#include "interpreter/interpreter.h"

namespace WasmEdge {
namespace Interpreter {

Expect<void>
Interpreter::runTableGetOp(Runtime::Instance::TableInstance &TabInst,
                           const AST::Instruction &Instr) {
  /// Pop Idx from Stack.
  uint32_t Idx = StackMgr.pop().get<uint32_t>();

  /// Get table[Idx] and push to Stack.
  if (auto Res = TabInst.getRefAddr(Idx)) {
    StackMgr.push(Res->get<UnknownRef>());
  } else {
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()}));
    return Unexpect(Res);
  }
  return {};
}

Expect<void>
Interpreter::runTableSetOp(Runtime::Instance::TableInstance &TabInst,
                           const AST::Instruction &Instr) {
  /// Pop Ref from Stack.
  RefVariant Ref = StackMgr.pop().get<UnknownRef>();

  /// Pop Idx from Stack.
  uint32_t Idx = StackMgr.pop().get<uint32_t>();

  /// Set table[Idx] with Ref.
  if (auto Res = TabInst.setRefAddr(Idx, Ref); !Res) {
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()}));
    return Unexpect(Res);
  }
  return {};
}

Expect<void>
Interpreter::runTableInitOp(Runtime::Instance::TableInstance &TabInst,
                            Runtime::Instance::ElementInstance &ElemInst,
                            const AST::Instruction &Instr) {
  /// Pop the length, source, and destination from stack.
  uint32_t Len = StackMgr.pop().get<uint32_t>();
  uint32_t Src = StackMgr.pop().get<uint32_t>();
  uint32_t Dst = StackMgr.pop().get<uint32_t>();

  /// Replace tab[Dst : Dst + Len] with elem[Src : Src + Len].
  if (auto Res = TabInst.setRefs(ElemInst.getRefs(), Dst, Src, Len)) {
    return {};
  } else {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }
}

Expect<void>
Interpreter::runElemDropOp(Runtime::Instance::ElementInstance &ElemInst) {
  /// Clear element instance.
  ElemInst.clear();
  return {};
}

Expect<void>
Interpreter::runTableCopyOp(Runtime::Instance::TableInstance &TabInstDst,
                            Runtime::Instance::TableInstance &TabInstSrc,
                            const AST::Instruction &Instr) {
  /// Pop the length, source, and destination from stack.
  uint32_t Len = StackMgr.pop().get<uint32_t>();
  uint32_t Src = StackMgr.pop().get<uint32_t>();
  uint32_t Dst = StackMgr.pop().get<uint32_t>();

  /// Replace tab_dst[Dst : Dst + Len] with tab_src[Src : Src + Len].
  if (auto Refs = TabInstSrc.getRefs(Src, Len)) {
    if (auto Res = TabInstDst.setRefs(*Refs, Dst, 0, Len)) {
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
Interpreter::runTableGrowOp(Runtime::Instance::TableInstance &TabInst) {
  /// Pop N for growing size, Val for init ref value.
  uint32_t N = StackMgr.pop().get<uint32_t>();
  ValVariant &Val = StackMgr.getTop();

  /// Grow size and push result.
  const uint32_t CurrSize = TabInst.getSize();

  if (TabInst.growTable(N, Val.get<UnknownRef>())) {
    Val.emplace<uint32_t>(CurrSize);
  } else {
    Val.emplace<int32_t>(INT32_C(-1));
  }
  return {};
}

Expect<void>
Interpreter::runTableSizeOp(Runtime::Instance::TableInstance &TabInst) {
  /// Push SZ = size to stack.
  StackMgr.push(TabInst.getSize());
  return {};
}

Expect<void>
Interpreter::runTableFillOp(Runtime::Instance::TableInstance &TabInst,
                            const AST::Instruction &Instr) {
  /// Pop the length, ref_value, and offset from stack.
  uint32_t Len = StackMgr.pop().get<uint32_t>();
  RefVariant Val = StackMgr.pop().get<UnknownRef>();
  uint32_t Off = StackMgr.pop().get<uint32_t>();

  /// Fill refs with ref_value.
  if (auto Res = TabInst.fillRefs(Val, Off, Len)) {
    return {};
  } else {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(Res);
  }
}

} // namespace Interpreter
} // namespace WasmEdge
