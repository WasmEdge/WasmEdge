#include "executor/common.h"
#include "executor/worker.h"
#include "executor/worker/util.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {

ErrCode Worker::runBlockOp(AST::Instruction *Instr) {
  /// Check the instruction type.
  auto BlockInstr = dynamic_cast<AST::BlockControlInstruction *>(Instr);
  if (BlockInstr == nullptr || BlockInstr->getOpCode() != OpCode::Block)
    return ErrCode::InstructionTypeMismatch;

  /// Get result type for arity.
  AST::ValType ResultType = BlockInstr->getResultType();
  unsigned int Arity = (ResultType == AST::ValType::None) ? 0 : 1;

  /// Create Label{ nothing } and push.
  return enterBlock(Arity, nullptr, BlockInstr->getBody());
}

ErrCode Worker::runLoopOp(AST::Instruction *Instr) {
  /// Check the instruction type.
  auto LoopInstr = dynamic_cast<AST::BlockControlInstruction *>(Instr);
  if (LoopInstr == nullptr || LoopInstr->getOpCode() != OpCode::Loop)
    return ErrCode::InstructionTypeMismatch;

  /// Get result type for arity.
  AST::ValType ResultType = LoopInstr->getResultType();

  /// Create Label{ loop-instruction } and push.
  return enterBlock(0, Instr, LoopInstr->getBody());
}

ErrCode Worker::runIfElseOp(AST::Instruction *Instr) {
  /// Check the instruction type.
  auto IfElseInstr = dynamic_cast<AST::IfElseControlInstruction *>(Instr);
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
  if (retrieveValue<uint32_t>(*Val.get()) != 0) {
    const AST::InstrVec &IfStatement = IfElseInstr->getIfStatement();
    if (IfStatement.size() > 0) {
      Status = enterBlock(Arity, nullptr, IfStatement);
    }
  } else {
    const AST::InstrVec &ElseStatement = IfElseInstr->getElseStatement();
    if (ElseStatement.size() > 0) {
      Status = enterBlock(Arity, nullptr, ElseStatement);
    }
  }
  return Status;
}

ErrCode Worker::runBrOp(AST::Instruction *Instr) {
  /// Check the instruction type.
  auto BrInstr = dynamic_cast<AST::BrControlInstruction *>(Instr);
  if (BrInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }
  return branchToLabel(BrInstr->getLabelIndex());
}

ErrCode Worker::runBrIfOp(AST::Instruction *Instr) {
  auto Status = ErrCode::Success;
  std::unique_ptr<ValueEntry> Val;
  StackMgr.pop(Val);
  if (retrieveValue<uint32_t>(*Val.get()) != 0) {
    Status = runBrOp(Instr);
  }
  return Status;
}

ErrCode Worker::runBrTableOp(AST::Instruction *Instr) {
  /// Check the instruction type.
  auto BrTableInstr = dynamic_cast<AST::BrTableControlInstruction *>(Instr);
  if (BrTableInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  /// Get value on top of stack.
  auto Status = ErrCode::Success;
  std::unique_ptr<ValueEntry> Val;
  StackMgr.pop(Val);
  int32_t Value = retrieveValue<uint32_t>(*Val.get());

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

ErrCode Worker::runCallOp(AST::Instruction *Instr) {
  /// Check the instruction type.
  auto CallInstr = dynamic_cast<AST::CallControlInstruction *>(Instr);
  if (CallInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  /// Get Function address.
  unsigned int ModuleAddr = CurrentFrame->getModuleAddr();
  Instance::ModuleInstance *ModuleInst = nullptr;
  StoreMgr.getModule(ModuleAddr, ModuleInst);
  unsigned int FuncAddr;
  ModuleInst->getFuncAddr(CallInstr->getIndex(), FuncAddr);
  return invokeFunction(FuncAddr);
}

ErrCode Worker::runCallIndirectOp(AST::Instruction *Instr) {
  /// Check the instruction type.
  auto CallInstr = dynamic_cast<AST::CallControlInstruction *>(Instr);
  if (CallInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  /// Get Table Instance
  ErrCode Status = ErrCode::Success;
  Instance::TableInstance *TableInst = nullptr;
  if ((Status = getTabInstByIdx(0, TableInst)) != ErrCode::Success) {
    return Status;
  };

  /// Get function type at index x.
  unsigned int ModuleAddr = CurrentFrame->getModuleAddr();
  Instance::ModuleInstance *ModuleInst = nullptr;
  Instance::ModuleInstance::FType *FuncType = nullptr;
  StoreMgr.getModule(ModuleAddr, ModuleInst);
  if ((Status = ModuleInst->getFuncType(CallInstr->getIndex(), FuncType)) !=
      ErrCode::Success) {
    return Status;
  };

  /// Pop the value i32.const i from the Stack.
  std::unique_ptr<ValueEntry> Idx;
  StackMgr.pop(Idx);

  /// Get function address.
  unsigned int FuncAddr;
  if ((Status = TableInst->getElemAddr(retrieveValue<uint32_t>(*Idx.get()),
                                       FuncAddr)) != ErrCode::Success) {
    return Status;
  };

  /// Check function type.
  Instance::FunctionInstance *FuncInst = nullptr;
  if ((Status = StoreMgr.getFunction(FuncAddr, FuncInst)) != ErrCode::Success) {
    return Status;
  }
  Instance::ModuleInstance::FType *DstFuncType = FuncInst->getFuncType();
  if (FuncType->Params.size() != DstFuncType->Params.size() ||
      FuncType->Returns.size() != DstFuncType->Returns.size()) {
    return ErrCode::TypeNotMatch;
  }
  for (unsigned int I = 0; I < FuncType->Params.size(); I++) {
    if (FuncType->Params[I] != DstFuncType->Params[I]) {
      return ErrCode::TypeNotMatch;
    }
  }
  for (unsigned int I = 0; I < FuncType->Returns.size(); I++) {
    if (FuncType->Returns[I] != DstFuncType->Returns[I]) {
      return ErrCode::TypeNotMatch;
    }
  }

  return invokeFunction(FuncAddr);
}

} // namespace Executor
} // namespace SSVM
