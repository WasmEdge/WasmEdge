// SPDX-License-Identifier: Apache-2.0
#include "common/value.h"
#include "interpreter/interpreter.h"

#include <algorithm>

namespace SSVM {
namespace Interpreter {

Expect<void>
Interpreter::runBlockOp(const AST::BlockControlInstruction &Instr) {
  /// Get result type for arity.
  const ValType ResultType = Instr.getResultType();
  uint32_t Arity = (ResultType == ValType::None) ? 0 : 1;

  /// Create Label{ nothing } and push.
  return enterBlock(Arity, nullptr, Instr.getBody());
}

Expect<void> Interpreter::runLoopOp(const AST::BlockControlInstruction &Instr) {
  /// Create Label{ loop-instruction } and push.
  return enterBlock(0, &Instr, Instr.getBody());
}

Expect<void>
Interpreter::runIfElseOp(const AST::IfElseControlInstruction &Instr) {
  /// Get condition and result type for arity.
  ValVariant Cond = StackMgr.pop();
  ValType ResultType = Instr.getResultType();
  uint32_t Arity = (ResultType == ValType::None) ? 0 : 1;

  /// If non-zero, run if-statement; else, run else-statement.
  if (retrieveValue<uint32_t>(Cond) != 0) {
    const auto &IfStatement = Instr.getIfStatement();
    if (!IfStatement.empty()) {
#ifndef ONNC_WASM
      /// If-then case should add the cost.
      if (!Measure->addInstrCost(OpCode::Else)) {
        return Unexpect(ErrCode::CostLimitExceeded);
      }
#endif
      return enterBlock(Arity, nullptr, IfStatement);
    }
  } else {
    const auto &ElseStatement = Instr.getElseStatement();
    if (!ElseStatement.empty()) {
#ifndef ONNC_WASM
      /// If-then case should add the cost.
      if (!Measure->addInstrCost(OpCode::Else)) {
        return Unexpect(ErrCode::CostLimitExceeded);
      }
#endif
      return enterBlock(Arity, nullptr, ElseStatement);
    }
  }
  return {};
}

Expect<void> Interpreter::runBrOp(const AST::BrControlInstruction &Instr) {
  return branchToLabel(Instr.getLabelIndex());
}

Expect<void> Interpreter::runBrIfOp(const AST::BrControlInstruction &Instr) {
  ValVariant Cond = StackMgr.pop();
  if (retrieveValue<uint32_t>(Cond) != 0) {
    return runBrOp(Instr);
  }
  return {};
}

Expect<void>
Interpreter::runBrTableOp(const AST::BrTableControlInstruction &Instr) {
  /// Get value on top of stack.
  uint32_t Value = retrieveValue<uint32_t>(StackMgr.pop());

  /// Do branch.
  const auto &LabelTable = Instr.getLabelTable();
  if (Value < LabelTable.size()) {
    return branchToLabel(LabelTable[Value]);
  }
  return branchToLabel(Instr.getLabelIndex());
}

Expect<void> Interpreter::runReturnOp() { return leaveFunction(); }

Expect<void> Interpreter::runCallOp(Runtime::StoreManager &StoreMgr,
                                    const AST::CallControlInstruction &Instr) {
  /// Get Function address.
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  const uint32_t FuncAddr = *ModInst->getFuncAddr(Instr.getFuncIndex());
  const auto *FuncInst = *StoreMgr.getFunction(FuncAddr);
  return enterFunction(StoreMgr, *FuncInst);
}

Expect<void>
Interpreter::runCallIndirectOp(Runtime::StoreManager &StoreMgr,
                               const AST::CallControlInstruction &Instr) {
  /// Get Table Instance
  const auto *TabInst = getTabInstByIdx(StoreMgr, 0);

  /// Get function type at index x.
  const auto *ModInst = *StoreMgr.getModule(StackMgr.getModuleAddr());
  const auto *TargetFuncType = *ModInst->getFuncType(Instr.getFuncIndex());

  /// Pop the value i32.const i from the Stack.
  ValVariant Idx = StackMgr.pop();

  /// Get function address.
  uint32_t FuncAddr;
  if (auto Res = TabInst->getElemAddr(retrieveValue<uint32_t>(Idx))) {
    FuncAddr = *Res;
  } else {
    return Unexpect(Res);
  }

  /// Check function type.
  const auto *FuncInst = *StoreMgr.getFunction(FuncAddr);
  const auto &FuncType = FuncInst->getFuncType();
  if (TargetFuncType->Params != FuncType.Params ||
      TargetFuncType->Returns != FuncType.Returns) {
    return Unexpect(ErrCode::TypeNotMatch);
  }
  return enterFunction(StoreMgr, *FuncInst);
}

} // namespace Interpreter
} // namespace SSVM
