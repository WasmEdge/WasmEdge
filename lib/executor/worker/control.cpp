#include "executor/common.h"
#include "executor/worker.h"
#include "executor/worker/util.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {

ErrCode Worker::runBlockOp(AST::ControlInstruction *InstrPtr) {
  /// Check the instruction type.
  auto BlockInstr = dynamic_cast<AST::BlockControlInstruction *>(InstrPtr);
  if (BlockInstr == nullptr || BlockInstr->getOpCode() != OpCode::Block)
    return ErrCode::InstructionTypeMismatch;

  /// Get result type for arity.
  AST::ValType ResultType = BlockInstr->getResultType();
  unsigned int Arity = (ResultType == AST::ValType::None) ? 0 : 1;

  /// Create Label{ nothing } and push.
  return enterBlock(Arity, nullptr, BlockInstr->getBody());
}

ErrCode Worker::runLoopOp(AST::ControlInstruction *InstrPtr) {
  /// Check the instruction type.
  auto LoopInstr = dynamic_cast<AST::BlockControlInstruction *>(InstrPtr);
  if (LoopInstr == nullptr || LoopInstr->getOpCode() != OpCode::Loop)
    return ErrCode::InstructionTypeMismatch;

  /// Get result type for arity.
  AST::ValType ResultType = LoopInstr->getResultType();

  /// Create Label{ loop-instruction } and push.
  return enterBlock(0, InstrPtr, LoopInstr->getBody());
}

ErrCode Worker::runIfElseOp(AST::ControlInstruction *InstrPtr) {
  /// Check the instruction type.
  auto IfElseInstr = dynamic_cast<AST::IfElseControlInstruction *>(InstrPtr);
  if (IfElseInstr == nullptr)
    return ErrCode::InstructionTypeMismatch;

  /// Get value on top of stack
  auto Status = ErrCode::Success;
  std::unique_ptr<ValueEntry> Val;
  StackMgr.pop(Val);

  /// Get result type for arity.
  AST::ValType ResultType = IfElseInstr->getResultType();
  unsigned int Arity = (ResultType == AST::ValType::None) ? 0 : 1;

  /// If non-zero, run if-statement; else, run else-statement.
  if (retrieveValue<int32_t>(*Val.get()) != 0) {
    const InstrVec &IfStatement = IfElseInstr->getIfStatement();
    if (IfStatement.size() > 0) {
      Status = enterBlock(Arity, nullptr, IfStatement);
    }
  } else {
    const InstrVec &ElseStatement = IfElseInstr->getElseStatement();
    if (ElseStatement.size() > 0) {
      Status = enterBlock(Arity, nullptr, ElseStatement);
    }
  }
  return Status;
}

ErrCode Worker::runBrOp(AST::ControlInstruction *InstrPtr) {
  /// Check the instruction type.
  auto BrInstr = dynamic_cast<AST::BrControlInstruction *>(InstrPtr);
  if (BrInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }
  return branchToLabel(BrInstr->getLabelIndex());
}

ErrCode Worker::runBrIfOp(AST::ControlInstruction *InstrPtr) {
  auto Status = ErrCode::Success;
  std::unique_ptr<ValueEntry> Val;
  StackMgr.pop(Val);
  if (retrieveValue<int32_t>(*Val.get()) != 0) {
    Status = runBrOp(InstrPtr);
  }
  return Status;
}

ErrCode Worker::runBrTableOp(AST::ControlInstruction *InstrPtr) {
  /// Check the instruction type.
  auto BrTableInstr = dynamic_cast<AST::BrTableControlInstruction *>(InstrPtr);
  if (BrTableInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  /// Get value on top of stack.
  auto Status = ErrCode::Success;
  std::unique_ptr<ValueEntry> Val;
  StackMgr.pop(Val);
  int32_t Value = retrieveValue<int32_t>(*Val.get());

  /// Do branch.
  const std::vector<unsigned int> &LabelTable = BrTableInstr->getLabelTable();
  if (Value < LabelTable.size()) {
    Status = branchToLabel(LabelTable[Value]);
  } else {
    Status = branchToLabel(BrTableInstr->getLabelIdx());
  }
  return Status;
}

ErrCode Worker::runReturnOp() { return returnFunction(); }

ErrCode Worker::runCallOp(AST::ControlInstruction *InstrPtr) {
  /// Check the instruction type.
  auto CallInstr = dynamic_cast<AST::CallControlInstruction *>(InstrPtr);
  if (CallInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  /// Get current frame.
  auto Status = ErrCode::Success;
  if ((Status = StackMgr.getCurrentFrame(CurrentFrame)) != ErrCode::Success) {
    return Status;
  }

  /// Get Function address.
  unsigned int ModuleAddr = CurrentFrame->getModuleAddr();
  Instance::ModuleInstance *ModuleInst = nullptr;
  StoreMgr.getModule(ModuleAddr, ModuleInst);
  unsigned int FuncAddr;
  ModuleInst->getFuncAddr(CallInstr->getIndex(), FuncAddr);
  return invokeFunction(FuncAddr);
}

} // namespace Executor
} // namespace SSVM
