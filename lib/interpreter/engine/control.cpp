// SPDX-License-Identifier: Apache-2.0
#include "common/value.h"
#include "interpreter/interpreter.h"

#include <algorithm>

namespace SSVM {
namespace Interpreter {

Expect<void> Interpreter::runBlockOp(Runtime::StoreManager &StoreMgr,
                                     const AST::Instruction &Instr,
                                     AST::InstrView::iterator &PC) {
  /// Get result type for arity.
  auto BlockSig = getBlockArity(StoreMgr, Instr.getBlockType());
  AST::InstrView::iterator Cont = PC + Instr.getJumpEnd();

  /// Create Label{ nothing } and push.
  StackMgr.pushLabel(BlockSig.first, BlockSig.second, Cont);
  return {};
}

Expect<void> Interpreter::runLoopOp(Runtime::StoreManager &StoreMgr,
                                    const AST::Instruction &Instr,
                                    AST::InstrView::iterator &PC) {
  /// Get result type for arity.
  auto BlockSig = getBlockArity(StoreMgr, Instr.getBlockType());
  AST::InstrView::iterator Cont = PC + Instr.getJumpEnd();

  /// Create Label{ loop-instruction } and push.
  StackMgr.pushLabel(BlockSig.first, BlockSig.first, Cont, PC);
  return {};
}

Expect<void> Interpreter::runIfElseOp(Runtime::StoreManager &StoreMgr,
                                      const AST::Instruction &Instr,
                                      AST::InstrView::iterator &PC) {
  /// Get condition.
  uint32_t Cond = retrieveValue<uint32_t>(StackMgr.pop());

  /// Get result type for arity.
  auto BlockSig = getBlockArity(StoreMgr, Instr.getBlockType());
  AST::InstrView::iterator Cont = PC + Instr.getJumpEnd();

  /// If non-zero, run if-statement; else, run else-statement.
  if (Cond == 0) {
    if (Instr.getJumpElse() == Instr.getJumpEnd()) {
      /// No else-statement case. Jump to right before End instruction.
      PC += (Instr.getJumpEnd() - 1);
    } else {
      if (Stat) {
        Stat->incInstrCount();
        if (unlikely(!Stat->addInstrCost(OpCode::Else))) {
          return Unexpect(ErrCode::CostLimitExceeded);
        }
      }
      /// Have else-statement case. Jump to Else instruction to continue.
      PC += Instr.getJumpElse();
    }
  }
  StackMgr.pushLabel(BlockSig.first, BlockSig.second, Cont);
  return {};
}

Expect<void> Interpreter::runBrOp(Runtime::StoreManager &StoreMgr,
                                  const AST::Instruction &Instr,
                                  AST::InstrView::iterator &PC) {
  return branchToLabel(StoreMgr, Instr.getTargetIndex(), PC);
}

Expect<void> Interpreter::runBrIfOp(Runtime::StoreManager &StoreMgr,
                                    const AST::Instruction &Instr,
                                    AST::InstrView::iterator &PC) {
  if (retrieveValue<uint32_t>(StackMgr.pop()) != 0) {
    return runBrOp(StoreMgr, Instr, PC);
  }
  return {};
}

Expect<void> Interpreter::runBrTableOp(Runtime::StoreManager &StoreMgr,
                                       const AST::Instruction &Instr,
                                       AST::InstrView::iterator &PC) {
  /// Get value on top of stack.
  uint32_t Value = retrieveValue<uint32_t>(StackMgr.pop());

  /// Do branch.
  const auto &LabelTable = Instr.getLabelList();
  if (Value < LabelTable.size()) {
    return branchToLabel(StoreMgr, LabelTable[Value], PC);
  }
  return branchToLabel(StoreMgr, Instr.getTargetIndex(), PC);
}

Expect<void> Interpreter::runReturnOp(AST::InstrView::iterator &PC) {
  PC = StackMgr.getBottomLabel().From;
  StackMgr.popFrame();
  return {};
}

Expect<void> Interpreter::runCallOp(Runtime::StoreManager &StoreMgr,
                                    const AST::Instruction &Instr,
                                    AST::InstrView::iterator &PC) {
  /// Get Function address.
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  const uint32_t FuncAddr = *ModInst->getFuncAddr(Instr.getTargetIndex());
  const auto *FuncInst = *StoreMgr.getFunction(FuncAddr);
  if (auto Res = enterFunction(StoreMgr, *FuncInst, PC); !Res) {
    return Unexpect(Res);
  } else {
    PC = (*Res) - 1;
  }
  return {};
}

Expect<void> Interpreter::runCallIndirectOp(Runtime::StoreManager &StoreMgr,
                                            const AST::Instruction &Instr,
                                            AST::InstrView::iterator &PC) {
  /// Get Table Instance
  const auto *TabInst = getTabInstByIdx(StoreMgr, Instr.getSourceIndex());

  /// Get function type at index x.
  const auto *ExpModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  const auto *ExpFuncType = *ExpModInst->getFuncType(Instr.getTargetIndex());

  /// Pop the value i32.const i from the Stack.
  uint32_t Idx = retrieveValue<uint32_t>(StackMgr.pop());

  /// If idx not small than tab.elem, trap.
  if (Idx >= TabInst->getSize()) {
    LOG(ERROR) << ErrCode::UndefinedElement;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()});
    return Unexpect(ErrCode::UndefinedElement);
  }

  /// Get function address.
  ValVariant Ref = *TabInst->getRefAddr(Idx);
  if (isNullRef(Ref)) {
    LOG(ERROR) << ErrCode::UninitializedElement;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Idx},
                                           {ValTypeFromType<uint32_t>()});
    return Unexpect(ErrCode::UninitializedElement);
  }
  uint32_t FuncAddr = retrieveFuncIdx(Ref);

  /// Check function type.
  const auto *FuncInst = *StoreMgr.getFunction(FuncAddr);
  if (FuncInst->isHostFunction()) {
    const auto *FuncType = &FuncInst->getHostFunc().getFuncType();
    if (*ExpFuncType != *FuncType) {
      LOG(ERROR) << ErrCode::IndirectCallTypeMismatch;
      LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                             Instr.getOffset(), {Idx},
                                             {ValTypeFromType<uint32_t>()});
      LOG(ERROR) << ErrInfo::InfoMismatch(ExpFuncType->Params,
                                          ExpFuncType->Returns,
                                          FuncType->Params, FuncType->Returns);
      return Unexpect(ErrCode::IndirectCallTypeMismatch);
    }
  } else {
    const auto *ModInst = *StoreMgr.getModule(FuncInst->getModuleAddr());
    const auto *FuncType = *ModInst->getFuncType(FuncInst->getFuncTypeIndex());
    if (*ExpFuncType != *FuncType) {
      LOG(ERROR) << ErrCode::IndirectCallTypeMismatch;
      LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                             Instr.getOffset(), {Idx},
                                             {ValTypeFromType<uint32_t>()});
      LOG(ERROR) << ErrInfo::InfoMismatch(ExpFuncType->Params,
                                          ExpFuncType->Returns,
                                          FuncType->Params, FuncType->Returns);
      return Unexpect(ErrCode::IndirectCallTypeMismatch);
    }
  }

  /// Enter function and move PC.
  if (auto Res = enterFunction(StoreMgr, *FuncInst, PC); !Res) {
    return Unexpect(Res);
  } else {
    PC = (*Res) - 1;
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
