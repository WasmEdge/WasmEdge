// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runIfElseOp(Runtime::StackManager &StackMgr,
                                   const AST::Instruction &Instr,
                                   AST::InstrView::iterator &PC) noexcept {
  // Get condition.
  uint32_t Cond = StackMgr.pop().get<uint32_t>();

  // If non-zero, run if-statement; else, run else-statement.
  if (Cond == 0) {
    if (Instr.getJumpElse() == Instr.getJumpEnd()) {
      // No else-statement case. Jump to right before End instruction.
      PC += (Instr.getJumpEnd() - 1);
    } else {
      if (Stat) {
        Stat->incInstrCount();
        if (unlikely(!Stat->addInstrCost(OpCode::Else))) {
          return Unexpect(ErrCode::CostLimitExceeded);
        }
      }
      // Have else-statement case. Jump to Else instruction to continue.
      PC += Instr.getJumpElse();
    }
  }
  return {};
}

Expect<void> Executor::runBrOp(Runtime::StackManager &StackMgr,
                               const AST::Instruction &Instr,
                               AST::InstrView::iterator &PC) noexcept {
  return branchToLabel(StackMgr, Instr.getJump().StackEraseBegin,
                       Instr.getJump().StackEraseEnd, Instr.getJump().PCOffset,
                       PC);
}

Expect<void> Executor::runBrIfOp(Runtime::StackManager &StackMgr,
                                 const AST::Instruction &Instr,
                                 AST::InstrView::iterator &PC) noexcept {
  if (StackMgr.pop().get<uint32_t>() != 0) {
    return runBrOp(StackMgr, Instr, PC);
  }
  return {};
}

Expect<void> Executor::runBrTableOp(Runtime::StackManager &StackMgr,
                                    const AST::Instruction &Instr,
                                    AST::InstrView::iterator &PC) noexcept {
  // Get value on top of stack.
  uint32_t Value = StackMgr.pop().get<uint32_t>();

  // Do branch.
  auto LabelTable = Instr.getLabelList();
  if (Value < LabelTable.size()) {
    return branchToLabel(StackMgr, LabelTable[Value].StackEraseBegin,
                         LabelTable[Value].StackEraseEnd,
                         LabelTable[Value].PCOffset, PC);
  }
  return branchToLabel(StackMgr, Instr.getJump().StackEraseBegin,
                       Instr.getJump().StackEraseEnd, Instr.getJump().PCOffset,
                       PC);
}

Expect<void> Executor::runReturnOp(Runtime::StackManager &StackMgr,
                                   const AST::Instruction &Instr,
                                   AST::InstrView::iterator &PC) noexcept {
  if (auto Res = branchToLabel(StackMgr, Instr.getJump().StackEraseBegin,
                               Instr.getJump().StackEraseEnd,
                               Instr.getJump().PCOffset, PC);
      !Res) {
    return Unexpect(Res);
  }
  PC = StackMgr.popFrame();
  return {};
}

Expect<void> Executor::runCallOp(Runtime::StoreManager &StoreMgr,
                                 Runtime::StackManager &StackMgr,
                                 const AST::Instruction &Instr,
                                 AST::InstrView::iterator &PC) noexcept {
  // Get Function address.
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  const uint32_t FuncAddr = *ModInst->getFuncAddr(Instr.getTargetIndex());
  const auto *FuncInst = *StoreMgr.getFunction(FuncAddr);
  if (auto Res = enterFunction(StoreMgr, StackMgr, *FuncInst, PC + 1); !Res) {
    return Unexpect(Res);
  } else {
    PC = (*Res) - 1;
  }
  return {};
}

Expect<void> Executor::runCallIndirectOp(
    Runtime::StoreManager &StoreMgr, Runtime::StackManager &StackMgr,
    const AST::Instruction &Instr, AST::InstrView::iterator &PC) noexcept {
  // Get Table Instance
  const auto *TabInst =
      getTabInstByIdx(StoreMgr, StackMgr, Instr.getSourceIndex());

  // Get function type at index x.
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  const auto *TargetFuncType = *ModInst->getFuncType(Instr.getTargetIndex());

  // Pop the value i32.const i from the Stack.
  uint32_t Idx = StackMgr.pop().get<uint32_t>();

  // If idx not small than tab.elem, trap.
  if (Idx >= TabInst->getSize()) {
    spdlog::error(ErrCode::UndefinedElement);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()}));
    return Unexpect(ErrCode::UndefinedElement);
  }

  // Get function address.
  ValVariant Ref = TabInst->getRefAddr(Idx)->get<UnknownRef>();
  if (isNullRef(Ref)) {
    spdlog::error(ErrCode::UninitializedElement);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()}));
    return Unexpect(ErrCode::UninitializedElement);
  }
  uint32_t FuncAddr = retrieveFuncIdx(Ref);

  // Check function type.
  const auto *FuncInst = *StoreMgr.getFunction(FuncAddr);
  const auto &FuncType = FuncInst->getFuncType();
  if (*TargetFuncType != FuncType) {
    spdlog::error(ErrCode::IndirectCallTypeMismatch);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()}));
    spdlog::error(ErrInfo::InfoMismatch(
        TargetFuncType->getParamTypes(), TargetFuncType->getReturnTypes(),
        FuncType.getParamTypes(), FuncType.getReturnTypes()));
    return Unexpect(ErrCode::IndirectCallTypeMismatch);
  }
  if (auto Res = enterFunction(StoreMgr, StackMgr, *FuncInst, PC + 1); !Res) {
    return Unexpect(Res);
  } else {
    PC = (*Res) - 1;
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
