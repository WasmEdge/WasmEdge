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

ErrCode Worker::runBrOp(AST::ControlInstruction *InstrPtr) {
  /// Check the instruction type.
  auto BrInstr = dynamic_cast<AST::BrControlInstruction *>(InstrPtr);
  if (BrInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  /// Get the l-th label from top of stack and the continuation instruction.
  ErrCode Status = ErrCode::Success;
  LabelEntry *Label;
  AST::Instruction *ContInstr;
  if ((Status = StackMgr.getLabelWithCount(Label, BrInstr->getLabelIndex())) !=
      ErrCode::Success) {
    return Status;
  }
  ContInstr = Label->getTarget();

  /// Get arity of L and pop n values.
  unsigned int Arity = Label->getArity();
  std::vector<std::unique_ptr<ValueEntry>> Vals;
  for (unsigned int I = 0; I < Arity; I++) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    Vals.push_back(std::move(Val));
  }

  /// Repeat LabelIndex + 1 times
  for (unsigned int I = 0; I < BrInstr->getLabelIndex() + 1; I++) {
    while (StackMgr.isTopValue()) {
      StackMgr.pop();
    }
    /// Pop label entry and the corresponding instruction sequence.
    InstrPdr.popInstrs();
    StackMgr.pop();
  }

  /// Push the Vals back into the Stack
  for (auto Iter = Vals.rbegin(); Iter != Vals.rend(); Iter++) {
    std::unique_ptr<ValueEntry> Val = std::move(*Iter);
    StackMgr.push(Val);
  }

  /// Jump to the continuation of Label
  if (ContInstr != nullptr)
    Status = runLoopOp(dynamic_cast<AST::ControlInstruction *>(ContInstr));
  return Status;
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
