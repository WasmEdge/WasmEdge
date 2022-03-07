// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runBlockOp(Runtime::StoreManager &StoreMgr,
                                  Runtime::StackManager &StackMgr,
                                  const AST::Instruction &Instr,
                                  AST::InstrView::iterator &PC) {
  // Get result type for arity.
  auto BlockSig = getBlockArity(StoreMgr, StackMgr, Instr.getBlockType());
  AST::InstrView::iterator Cont = PC + Instr.getJumpEnd();

  // Create Label{ nothing } and push.
  StackMgr.pushLabel(BlockSig.first, BlockSig.second, Cont);
  return {};
}

Expect<void> Executor::runLoopOp(Runtime::StoreManager &StoreMgr,
                                 Runtime::StackManager &StackMgr,
                                 const AST::Instruction &Instr,
                                 AST::InstrView::iterator &PC) {
  // Get result type for arity.
  auto BlockSig = getBlockArity(StoreMgr, StackMgr, Instr.getBlockType());
  AST::InstrView::iterator Cont = PC + Instr.getJumpEnd();

  // Create Label{ loop-instruction } and push.
  StackMgr.pushLabel(BlockSig.first, BlockSig.first, Cont, PC);
  return {};
}

Expect<void> Executor::runIfElseOp(Runtime::StoreManager &StoreMgr,
                                   Runtime::StackManager &StackMgr,
                                   const AST::Instruction &Instr,
                                   AST::InstrView::iterator &PC) {
  // Get condition.
  uint32_t Cond = StackMgr.pop().get<uint32_t>();

  // Get result type for arity.
  auto BlockSig = getBlockArity(StoreMgr, StackMgr, Instr.getBlockType());
  AST::InstrView::iterator Cont = PC + Instr.getJumpEnd();

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
  StackMgr.pushLabel(BlockSig.first, BlockSig.second, Cont);
  return {};
}

Expect<void> Executor::runBrOp(Runtime::StoreManager &StoreMgr,
                               Runtime::StackManager &StackMgr,
                               const AST::Instruction &Instr,
                               AST::InstrView::iterator &PC) {
  return branchToLabel(StoreMgr, StackMgr, Instr.getTargetIndex(), PC);
}

Expect<void> Executor::runBrIfOp(Runtime::StoreManager &StoreMgr,
                                 Runtime::StackManager &StackMgr,
                                 const AST::Instruction &Instr,
                                 AST::InstrView::iterator &PC) {
  if (StackMgr.pop().get<uint32_t>() != 0) {
    return runBrOp(StoreMgr, StackMgr, Instr, PC);
  }
  return {};
}

Expect<void> Executor::runBrTableOp(Runtime::StoreManager &StoreMgr,
                                    Runtime::StackManager &StackMgr,
                                    const AST::Instruction &Instr,
                                    AST::InstrView::iterator &PC) {
  // Get value on top of stack.
  uint32_t Value = StackMgr.pop().get<uint32_t>();

  // Do branch.
  auto LabelTable = Instr.getLabelList();
  if (Value < LabelTable.size()) {
    return branchToLabel(StoreMgr, StackMgr, LabelTable[Value], PC);
  }
  return branchToLabel(StoreMgr, StackMgr, Instr.getTargetIndex(), PC);
}

Expect<void> Executor::runReturnOp(Runtime::StackManager &StackMgr,
                                   AST::InstrView::iterator &PC) {
  PC = StackMgr.popFrame();
  return {};
}

Expect<void> Executor::runCallOp(Runtime::StoreManager &StoreMgr,
                                 Runtime::StackManager &StackMgr,
                                 const AST::Instruction &Instr,
                                 AST::InstrView::iterator &PC,
                                 const bool IsTailCall) {
  // Get Function address.
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  const uint32_t FuncAddr = *ModInst->getFuncAddr(Instr.getTargetIndex());
  const auto *FuncInst = *StoreMgr.getFunction(FuncAddr);
  if (auto Res =
          enterFunction(StoreMgr, StackMgr, *FuncInst, PC + 1, IsTailCall);
      !Res) {
    return Unexpect(Res);
  } else {
    PC = (*Res) - 1;
  }
  return {};
}

Expect<void> Executor::runCallIndirectOp(Runtime::StoreManager &StoreMgr,
                                         Runtime::StackManager &StackMgr,
                                         const AST::Instruction &Instr,
                                         AST::InstrView::iterator &PC,
                                         const bool IsTailCall) {
  // Get Table Instance.
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

  // Enter the function.
  if (auto Res =
          enterFunction(StoreMgr, StackMgr, *FuncInst, PC + 1, IsTailCall);
      !Res) {
    return Unexpect(Res);
  } else {
    PC = (*Res) - 1;
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
