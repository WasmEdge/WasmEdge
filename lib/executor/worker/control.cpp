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
  /// Get result type for arity.
  AST::ValType ResultType = Instr->getResultType();
  unsigned int Arity = (ResultType == AST::ValType::None) ? 0 : 1;

  /// Create Label{ nothing } and push.
  return enterBlock(Arity, nullptr, *Instr->getBody());
}

ErrCode Worker::runLoopOp(AST::Instruction *Instr) {
  /// Get result type for arity.
  AST::ValType ResultType = Instr->getResultType();

  /// Create Label{ loop-instruction } and push.
  return enterBlock(0, Instr, *Instr->getBody());
}

ErrCode Worker::runIfElseOp(AST::Instruction *Instr) {
  /// Get value on top of stack
  auto Status = ErrCode::Success;
  std::unique_ptr<ValueEntry> Val;
  StackMgr.pop(Val);
  uint32_t Cond = retrieveValue<uint32_t>(*Val.get());
  MemPool.destroyValueEntry(std::move(Val));

  /// Get result type for arity.
  AST::ValType ResultType = Instr->getResultType();
  unsigned int Arity = (ResultType == AST::ValType::None) ? 0 : 1;

  /// If non-zero, run if-statement; else, run else-statement.
  if (Cond != 0) {
    const AST::InstrVec *IfStatement = Instr->getIfStatement();
    if (IfStatement->size() > 0) {
      Status = enterBlock(Arity, nullptr, *IfStatement);
    }
  } else {
    const AST::InstrVec *ElseStatement = Instr->getElseStatement();
    if (ElseStatement->size() > 0) {
      Status = enterBlock(Arity, nullptr, *ElseStatement);
    }
  }
  return Status;
}

ErrCode Worker::runBrOp(AST::Instruction *Instr) {
  return branchToLabel(Instr->getLabelIndex());
}

ErrCode Worker::runBrIfOp(AST::Instruction *Instr) {
  auto Status = ErrCode::Success;
  std::unique_ptr<ValueEntry> Val;
  StackMgr.pop(Val);
  if (retrieveValue<uint32_t>(*Val.get()) != 0) {
    Status = runBrOp(Instr);
  }
  MemPool.destroyValueEntry(std::move(Val));
  return Status;
}

ErrCode Worker::runBrTableOp(AST::Instruction *Instr) {
  /// Get value on top of stack.
  auto Status = ErrCode::Success;
  std::unique_ptr<ValueEntry> Val;
  StackMgr.pop(Val);
  int32_t Value = retrieveValue<uint32_t>(*Val.get());
  MemPool.destroyValueEntry(std::move(Val));

  /// Do branch.
  const std::vector<unsigned int> *LabelTable = Instr->getLabelTable();
  if (Value < LabelTable->size()) {
    Status = branchToLabel(LabelTable->at(Value));
  } else {
    Status = branchToLabel(Instr->getLabelIndex());
  }
  return Status;
}

ErrCode Worker::runReturnOp() { return returnFunction(); }

ErrCode Worker::runCallOp(AST::Instruction *Instr) {
  /// Get Function address.
  unsigned int ModuleAddr = CurrentFrame->getModuleAddr();
  Instance::ModuleInstance *ModuleInst = nullptr;
  StoreMgr.getModule(ModuleAddr, ModuleInst);
  unsigned int FuncAddr;
  ModuleInst->getFuncAddr(Instr->getFuncIndex(), FuncAddr);
  return invokeFunction(FuncAddr);
}

ErrCode Worker::runCallIndirectOp(AST::Instruction *Instr) {
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
  if ((Status = ModuleInst->getFuncType(Instr->getFuncIndex(), FuncType)) !=
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
  MemPool.destroyValueEntry(std::move(Idx));

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
