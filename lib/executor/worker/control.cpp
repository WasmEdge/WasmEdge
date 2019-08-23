#include "executor/common.h"
#include "executor/worker.h"
#include "executor/worker_util.h"

#include <cstdint>

namespace SSVM {
namespace Executor {

ErrCode Worker::runReturnOp() {
  StackMgr.getCurrentFrame(CurrentFrame);
  unsigned int Arity = CurrentFrame->getArity();
  std::vector<std::unique_ptr<ValueEntry>> Vals;
  for (int i = 0; i < Arity; i++) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    Vals.push_back(std::move(Val));
  }
  while (StackMgr.isTopFrame()) {
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

ErrCode Worker::runBrOp(AST::ControlInstruction *InstrPtr) {
  auto BrInstr = dynamic_cast<AST::BrControlInstruction *>(InstrPtr);
  if (BrInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }
  LabelEntry *Label;
  StackMgr.getLabelWithCount(Label, BrInstr->getLabelIndex());
  unsigned int Arity = Label->getArity();
  std::vector<std::unique_ptr<ValueEntry>> Vals;
  for (int i = 0; i < Arity; i++) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    Vals.push_back(std::move(Val));
  }
  /// Repeat LabelIndex+1 times
  for (int i = 0; i < BrInstr->getLabelIndex() + 1; i++) {
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

} // namespace Executor
} // namespace SSVM
