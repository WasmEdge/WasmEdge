// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

Expect<AST::InstrView::iterator>
Executor::runIfElseOp(Runtime::StackManager &StackMgr,
                      const AST::Instruction Instr,
                      AST::InstrView::iterator PC) noexcept {
  // Get condition.
  uint32_t Cond = StackMgr.pop().get<uint32_t>();

  // If non-zero, run if-statement; else, run else-statement.
  if (Cond == 0) {
    if (Instr.getJumpElse() == Instr.getJumpEnd()) {
      // No else-statement case. Jump to the End instruction.
      PC = Instr.getIterator() + Instr.getJumpEnd();
    } else {
      if (Stat) {
        Stat->incInstrCount();
        if (unlikely(!Stat->addInstrCost(OpCode::Else))) {
          return Unexpect(ErrCode::Value::CostLimitExceeded);
        }
      }
      // Else-statement case. Skip the Else instruction to continue.
      PC = Instr.getIterator() + Instr.getJumpElse();
      ++PC;
    }
  }
  return PC;
}

Expect<AST::InstrView::iterator>
Executor::runThrowOp(Runtime::StackManager &StackMgr,
                     const AST::Instruction Instr,
                     AST::InstrView::iterator PC) noexcept {
  auto *TagInst = getTagInstByIdx(StackMgr.getModule(), Instr.getTargetIndex());
  // The arguments will be popped from the stack in the throw function.
  return throwException(StackMgr, *TagInst, PC);
}

Expect<AST::InstrView::iterator>
Executor::runThrowRefOp(Runtime::StackManager &StackMgr,
                        const AST::Instruction Instr,
                        AST::InstrView::iterator PC) noexcept {
  const auto Ref = StackMgr.pop().get<RefVariant>();
  if (Ref.isNull()) {
    spdlog::error(ErrCode::Value::AccessNullException);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullException);
  }
  const auto *ExnInst = Ref.getPtr<Runtime::Instance::ExceptionInstance>();
  auto *TagInst = ExnInst->getTag();
  // Re-push the captured payload to mirror an initial `throw`, then unwind.
  StackMgr.pushValVec(ExnInst->getPayload());
  return throwException(StackMgr, *TagInst, PC, ExnInst);
}

Expect<AST::InstrView::iterator>
Executor::runBrOp(Runtime::StackManager &StackMgr,
                  const AST::Instruction Instr) noexcept {
  return branchToLabel(StackMgr, Instr.getJump(), Instr.getIterator());
}

Expect<AST::InstrView::iterator>
Executor::runBrIfOp(Runtime::StackManager &StackMgr,
                    const AST::Instruction Instr,
                    AST::InstrView::iterator PC) noexcept {
  if (StackMgr.pop().get<uint32_t>() != 0) {
    return runBrOp(StackMgr, Instr);
  }
  return PC;
}

Expect<AST::InstrView::iterator>
Executor::runBrOnNullOp(Runtime::StackManager &StackMgr,
                        const AST::Instruction Instr,
                        AST::InstrView::iterator PC) noexcept {
  if (StackMgr.getTop().get<RefVariant>().isNull()) {
    StackMgr.pop();
    return runBrOp(StackMgr, Instr);
  }
  return PC;
}

Expect<AST::InstrView::iterator>
Executor::runBrOnNonNullOp(Runtime::StackManager &StackMgr,
                           const AST::Instruction Instr,
                           AST::InstrView::iterator PC) noexcept {
  if (!StackMgr.getTop().get<RefVariant>().isNull()) {
    return runBrOp(StackMgr, Instr);
  }
  StackMgr.pop();
  return PC;
}

Expect<AST::InstrView::iterator>
Executor::runBrTableOp(Runtime::StackManager &StackMgr,
                       const AST::Instruction Instr) noexcept {
  // Get the value on top of the stack.
  uint32_t Value = StackMgr.pop().get<uint32_t>();

  // Do branch.
  auto LabelTable = Instr.getLabelList();
  const auto LabelTableSize = static_cast<uint32_t>(LabelTable.size() - 1);
  if (Value < LabelTableSize) {
    return branchToLabel(StackMgr, LabelTable[Value], Instr.getIterator());
  }
  return branchToLabel(StackMgr, LabelTable[LabelTableSize],
                       Instr.getIterator());
}

Expect<AST::InstrView::iterator>
Executor::runBrOnCastOp(Runtime::StackManager &StackMgr,
                        const AST::Instruction Instr,
                        AST::InstrView::iterator PC, bool IsReverse) noexcept {
  // Get the value on top of the stack.
  const auto &Val = StackMgr.getTop().get<RefVariant>();
  if (matchRef(StackMgr.getModule(), Instr.getBrCast().RType2, Val) !=
      IsReverse) {
    return branchToLabel(StackMgr, Instr.getBrCast().Jump, Instr.getIterator());
  }
  return PC;
}

Expect<AST::InstrView::iterator>
Executor::runReturnOp(Runtime::StackManager &StackMgr) noexcept {
  // Check stop token
  if (unlikely(StopToken.exchange(0, std::memory_order_relaxed))) {
    spdlog::error(ErrCode::Value::Interrupted);
    return Unexpect(ErrCode::Value::Interrupted);
  }
  return StackMgr.popFrame();
}

Expect<AST::InstrView::iterator>
Executor::runCallOp(Runtime::StackManager &StackMgr,
                    const AST::Instruction Instr, AST::InstrView::iterator PC,
                    bool IsTailCall) noexcept {
  // Get Function address. The PC has been advanced to the return position.
  const auto *FuncInst =
      getFuncInstByIdx(StackMgr.getModule(), Instr.getTargetIndex());
  return enterFunction(StackMgr, *FuncInst, PC, IsTailCall);
}

Expect<AST::InstrView::iterator>
Executor::runCallRefOp(Runtime::StackManager &StackMgr,
                       const AST::Instruction Instr,
                       AST::InstrView::iterator PC, bool IsTailCall) noexcept {
  const auto Ref = StackMgr.pop().get<RefVariant>();
  const auto *FuncInst = retrieveFuncRef(Ref);
  if (FuncInst == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullFunc);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullFunc);
  }

  // Get Function address. The PC has been advanced to the return position.
  return enterFunction(StackMgr, *FuncInst, PC, IsTailCall);
}

Expect<AST::InstrView::iterator> Executor::runCallIndirectOp(
    Runtime::StackManager &StackMgr, const AST::Instruction Instr,
    AST::InstrView::iterator PC, bool IsTailCall) noexcept {
  // Get Table Instance.
  const auto *TabInst =
      getTabInstByIdx(StackMgr.getModule(), Instr.getSourceIndex());

  // Get function type at index x.
  const auto *ModInst = StackMgr.getModule();
  const auto &ExpDefType = *ModInst->unsafeGetType(Instr.getTargetIndex());

  // Pop the value of index from the Stack.
  const auto AddrType = TabInst->getTableType().getLimit().getAddrType();
  uint64_t Idx = extractAddr(StackMgr.pop(), AddrType);

  // If idx not small than tab.elem, trap.
  if (Idx >= TabInst->getSize()) {
    spdlog::error(ErrCode::Value::UndefinedElement);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromAddressType(AddrType)}));
    return Unexpect(ErrCode::Value::UndefinedElement);
  }

  // Get function address. The bound is guaranteed.
  RefVariant Ref = *TabInst->getRefAddr(Idx);
  const auto *FuncInst = retrieveFuncRef(Ref);
  if (FuncInst == nullptr) {
    spdlog::error(ErrCode::Value::UninitializedElement);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromAddressType(AddrType)}));
    return Unexpect(ErrCode::Value::UninitializedElement);
  }

  // Check function type.
  if (!matchRef(ModInst, ValType(TypeCode::Ref, Instr.getTargetIndex()), Ref)) {
    auto &ExpFuncType = ExpDefType.getCompositeType().getFuncType();
    auto &GotFuncType = FuncInst->getFuncType();
    spdlog::error(ErrCode::Value::IndirectCallTypeMismatch);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromAddressType(AddrType)}));
    spdlog::error(ErrInfo::InfoMismatch(
        ExpFuncType.getParamTypes(), ExpFuncType.getReturnTypes(),
        GotFuncType.getParamTypes(), GotFuncType.getReturnTypes()));
    return Unexpect(ErrCode::Value::IndirectCallTypeMismatch);
  }

  // Enter the function. The PC has been advanced to the return position.
  return enterFunction(StackMgr, *FuncInst, PC, IsTailCall);
}

Expect<void> Executor::runTryTableOp(Runtime::StackManager &StackMgr,
                                     const AST::Instruction Instr) noexcept {
  const auto &TryDesc = Instr.getTryCatch();
  StackMgr.pushHandler(Instr.getIterator(), TryDesc.BlockParamNum,
                       TryDesc.getCatchList());
  return {};
}

} // namespace Executor
} // namespace WasmEdge
