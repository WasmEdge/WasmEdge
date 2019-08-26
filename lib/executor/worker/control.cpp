#include "executor/common.h"
#include "executor/worker.h"
#include "executor/worker_util.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {

ErrCode Worker::runBlockOp(AST::ControlInstruction *InstrPtr) {
  auto BlockInstr = dynamic_cast<AST::BlockControlInstruction *>(InstrPtr);
  if (BlockInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }
  std::unique_ptr<LabelEntry> Label =
      std::make_unique<LabelEntry>(/* Arity */ 1, BlockInstr->getBody());
  StackMgr.push(Label);
  std::unique_ptr<Worker> NewWorker =
      std::make_unique<Worker>(StoreMgr, StackMgr);
  NewWorker->setCode(BlockInstr->getBody());
  auto Status = NewWorker->run();
  if (NewWorker->getState() == State::Active) {
    std::vector<std::unique_ptr<ValueEntry>> Vals;
    while (!StackMgr.isTopLabel()) {
      std::unique_ptr<ValueEntry> Val = nullptr;
      StackMgr.pop(Val);
      Vals.push_back(std::move(Val));
    }
    /// Pop Label
    StackMgr.pop();
    /// Push the Vals into the Stack
    for (auto Iter = Vals.crbegin(); Iter != Vals.crend(); Iter++) {
      std::unique_ptr<ValueEntry> Val =
          std::make_unique<ValueEntry>(*Iter->get());
      StackMgr.push(Val);
    }
  }
  return Status;
}

ErrCode Worker::runBrOp(AST::ControlInstruction *InstrPtr) {
  auto BrInstr = dynamic_cast<AST::BrControlInstruction *>(InstrPtr);
  if (BrInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }
  LabelEntry *Label;
  StackMgr.getLabelWithCount(Label, BrInstr->getLabelIndex());
  unsigned int Arity = Label->getArity();
  std::vector<std::unique_ptr<ValueEntry>> Vals;
  for (unsigned int I = 0; I < Arity; I++) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    Vals.push_back(std::move(Val));
  }
  /// Repeat LabelIndex+1 times
  for (unsigned int I = 0; I < BrInstr->getLabelIndex() + 1; I++) {
    while (StackMgr.isTopValue()) {
      StackMgr.pop();
    }
    /// Pop label entry
    StackMgr.pop();
  }
  /// Push the Vals into the Stack
  for (auto Iter = Vals.crbegin(); Iter != Vals.crend(); Iter++) {
    std::unique_ptr<ValueEntry> Val =
        std::make_unique<ValueEntry>(*Iter->get());
    StackMgr.push(Val);
  }
  /// Jump to the continuation of Label
  std::unique_ptr<Worker> NewWorker =
      std::make_unique<Worker>(StoreMgr, StackMgr);
  NewWorker->setCode(Label->getInstructions());
  auto Status = NewWorker->run();
  TheState = State::Terminated;
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

ErrCode Worker::runReturnOp() {
  StackMgr.getCurrentFrame(CurrentFrame);
  unsigned int Arity = CurrentFrame->getArity();
  std::vector<std::unique_ptr<ValueEntry>> Vals;
  for (unsigned int I = 0; I < Arity; I++) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    Vals.push_back(std::move(Val));
  }
  while (!StackMgr.isTopFrame()) {
    StackMgr.pop();
  }
  /// Pop the frame entry from the Stack
  StackMgr.pop();
  /// Push the Vals into the Stack
  for (auto Iter = Vals.crbegin(); Iter != Vals.crend(); Iter++) {
    std::unique_ptr<ValueEntry> Val =
        std::make_unique<ValueEntry>(*Iter->get());
    StackMgr.push(Val);
  }
  /// Terminate this worker
  TheState = State::Terminated;

  return ErrCode::Success;
}

ErrCode Worker::runCallOp(AST::ControlInstruction *InstrPtr) {
  auto Status = ErrCode::Success;
  auto CallInstr = dynamic_cast<AST::CallControlInstruction *>(InstrPtr);
  if (CallInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }
  StackMgr.getCurrentFrame(CurrentFrame);

  /// Get Function Instance
  unsigned int ModuleAddr = CurrentFrame->getModuleAddr();
  Instance::ModuleInstance *ModuleInst = nullptr;
  StoreMgr.getModule(ModuleAddr, ModuleInst);
  unsigned int FuncAddr;
  ModuleInst->getFuncAddr(CallInstr->getIndex(), FuncAddr);
  Instance::FunctionInstance *FuncInst = nullptr;
  StoreMgr.getFunction(FuncAddr, FuncInst);

  /// Get function type
  Instance::ModuleInstance::FType *FuncType = nullptr;
  ModuleInst->getFuncType(FuncInst->getTypeIdx(), FuncType);

  /// Get function body instrs

  /// Pop argument vals
  std::vector<std::unique_ptr<ValueEntry>> Vals;
  for (unsigned int I = 0; I < FuncType->Params.size(); I++) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    Vals.push_back(std::move(Val));
  }
  std::reverse(Vals.begin(), Vals.end());

  /// Push frame with locals and args
  unsigned int Arity = FuncType->Returns.size();
  auto Frame = std::make_unique<FrameEntry>(ModuleAddr, /// Module address
                                            Arity,      /// Arity
                                            Vals,       /// Arguments
                                            FuncInst->getLocals() /// Local defs
  );
  StackMgr.push(Frame);

  /// Run block of function body
  /// TODO: refine it by call runBlockOp().
  std::unique_ptr<LabelEntry> Label =
      std::make_unique<LabelEntry>(Arity, FuncInst->getInstrs());
  StackMgr.push(Label);
  std::unique_ptr<Worker> NewWorker =
      std::make_unique<Worker>(StoreMgr, StackMgr);
  NewWorker->setCode(FuncInst->getInstrs());
  Status = NewWorker->run();
  if (NewWorker->getState() == State::Active) {
    std::vector<std::unique_ptr<ValueEntry>> Vals;
    while (!StackMgr.isTopLabel()) {
      std::unique_ptr<ValueEntry> Val = nullptr;
      StackMgr.pop(Val);
      Vals.push_back(std::move(Val));
    }
    /// Pop Label
    StackMgr.pop();

    /// Push the Vals into the Stack
    for (auto Iter = Vals.rbegin(); Iter != Vals.rend(); Iter++) {
      std::unique_ptr<ValueEntry> Val = std::move(*Iter);
      StackMgr.push(Val);
    }

    /// Pop the arity number of vals
    for (unsigned int I = 0; I < Arity; I++) {
      std::unique_ptr<ValueEntry> Val = nullptr;
      StackMgr.pop(Val);
      Vals.push_back(std::move(Val));
    }

    /// Pop frame.
    StackMgr.pop();

    /// Push the Vals into the Stack
    for (auto Iter = Vals.rbegin(); Iter != Vals.rend(); Iter++) {
      std::unique_ptr<ValueEntry> Val = std::move(*Iter);
      StackMgr.push(Val);
    }
  }

  return Status;
}

} // namespace Executor
} // namespace SSVM
