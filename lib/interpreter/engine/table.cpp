// SPDX-License-Identifier: Apache-2.0
#include "common/log.h"
#include "common/value.h"
#include "interpreter/interpreter.h"

namespace SSVM {
namespace Interpreter {

Expect<void>
Interpreter::runTableGetOp(Runtime::Instance::TableInstance &TabInst,
                           const AST::Instruction &Instr) {
  /// Pop Idx from Stack.
  uint32_t Idx = retrieveValue<uint32_t>(StackMgr.pop());

  /// Get table[Idx] and push to Stack.
  if (auto Res = TabInst.getRefAddr(Idx)) {
    StackMgr.push(*Res);
  } else {
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()});
    return Unexpect(Res);
  }
  return {};
}

Expect<void>
Interpreter::runTableSetOp(Runtime::Instance::TableInstance &TabInst,
                           const AST::Instruction &Instr) {
  /// Pop Ref from Stack.
  RefVariant Ref = retrieveValue<uint64_t>(StackMgr.pop());

  /// Pop Idx from Stack.
  uint32_t Idx = retrieveValue<uint32_t>(StackMgr.pop());

  /// Set table[Idx] with Ref.
  if (auto Res = TabInst.setRefAddr(Idx, Ref); !Res) {
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()});
    return Unexpect(Res);
  }
  return {};
}

Expect<void>
Interpreter::runTableInitOp(Runtime::Instance::TableInstance &TabInst,
                            Runtime::Instance::ElementInstance &ElemInst,
                            const AST::Instruction &Instr) {
  /// Pop the length, source, and destination from stack.
  uint32_t Len = retrieveValue<uint32_t>(StackMgr.pop());
  uint32_t Src = retrieveValue<uint32_t>(StackMgr.pop());
  uint32_t Dst = retrieveValue<uint32_t>(StackMgr.pop());

  /// Replace tab[Dst : Dst + Len] with elem[Src : Src + Len].
  if (auto Res = TabInst.setRefs(ElemInst.getRefs(), Dst, Src, Len)) {
    return {};
  } else {
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(Res);
  }
  return {};
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
  uint32_t Len = retrieveValue<uint32_t>(StackMgr.pop());
  uint32_t Src = retrieveValue<uint32_t>(StackMgr.pop());
  uint32_t Dst = retrieveValue<uint32_t>(StackMgr.pop());

  /// Replace tab_dst[Dst : Dst + Len] with tab_src[Src : Src + Len].
  if (auto Refs = TabInstSrc.getRefs(Src, Len)) {
    if (auto Res = TabInstDst.setRefs(*Refs, Dst, 0, Len)) {
      return {};
    } else {
      LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                             Instr.getOffset());
      return Unexpect(Res);
    }
  } else {
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(Refs);
  }
  return {};
}

Expect<void>
Interpreter::runTableGrowOp(Runtime::Instance::TableInstance &TabInst) {
  /// Pop N for growing size, Val for init ref value.
  uint32_t N = retrieveValue<uint32_t>(StackMgr.pop());
  ValVariant &Val = StackMgr.getTop();

  /// Grow size and push result.
  const uint32_t CurrSize = TabInst.getSize();

  if (TabInst.growTable(N, retrieveValue<uint64_t>(Val))) {
    Val = CurrSize;
  } else {
    Val = static_cast<uint32_t>(-1);
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
  uint32_t Len = retrieveValue<uint32_t>(StackMgr.pop());
  RefVariant Val = retrieveValue<uint64_t>(StackMgr.pop());
  uint32_t Off = retrieveValue<uint32_t>(StackMgr.pop());

  /// Fill refs with ref_value.
  if (auto Res = TabInst.fillRefs(Val, Off, Len)) {
    return {};
  } else {
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                           Instr.getOffset());
    return Unexpect(Res);
  }
}

} // namespace Interpreter
} // namespace SSVM
