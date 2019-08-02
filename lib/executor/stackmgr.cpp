#include "executor/stackmgr.h"

/// Getter of top frame entry. See "include/executor/stackmgr.h".
Executor::ErrCode StackMgr::getTop(FrameEntry *&Frame) {
  /// Check the size of stack.
  if (Stack.size() == 0)
    return Executor::ErrCode::StackEmpty;
  /// Check is the top entry a frame.
  if (Stack.back().index() != 0)
    return Executor::ErrCode::StackWrongEntry;
  /// Get the pointer of frame entry.
  Frame = std::get<0>(Stack.back()).get();
  return Executor::ErrCode::Success;
}

/// Getter of top label entry. See "include/executor/stackmgr.h".
Executor::ErrCode StackMgr::getTop(LabelEntry *&Label) {
  /// Check the size of stack.
  if (Stack.size() == 0)
    return Executor::ErrCode::StackEmpty;
  /// Check is the top entry a label.
  if (Stack.back().index() != 1)
    return Executor::ErrCode::StackWrongEntry;
  /// Get the pointer of label entry.
  Label = std::get<1>(Stack.back()).get();
  return Executor::ErrCode::Success;
}

/// Getter of top value entry. See "include/executor/stackmgr.h".
Executor::ErrCode StackMgr::getTop(ValueEntry *&Value) {
  /// Check the size of stack.
  if (Stack.size() == 0)
    return Executor::ErrCode::StackEmpty;
  /// Check is the top entry a value.
  if (Stack.back().index() != 2)
    return Executor::ErrCode::StackWrongEntry;
  /// Get the pointer of value entry.
  Value = std::get<2>(Stack.back()).get();
  return Executor::ErrCode::Success;
}

/// Push a new frame entry to stack. See "include/executor/stackmgr.h".
Executor::ErrCode StackMgr::push(std::unique_ptr<FrameEntry> &NewFrame) {
  Stack.push_back(std::move(NewFrame));
  FrameIdx.push_back(Stack.size() - 1);
  return Executor::ErrCode::Success;
}

/// Push a new label entry to stack. See "include/executor/stackmgr.h".
Executor::ErrCode StackMgr::push(std::unique_ptr<LabelEntry> &NewLabel) {
  Stack.push_back(std::move(NewLabel));
  LabelIdx.push_back(Stack.size() - 1);
  return Executor::ErrCode::Success;
}

/// Push a new value entry to stack. See "include/executor/stackmgr.h".
Executor::ErrCode StackMgr::push(std::unique_ptr<ValueEntry> &NewValue) {
  Stack.push_back(std::move(NewValue));
  return Executor::ErrCode::Success;
}

/// Pop and return the frame entry. See "include/executor/stackmgr.h".
Executor::ErrCode StackMgr::pop(std::unique_ptr<FrameEntry> &Frame) {
  /// Check the size of stack.
  if (Stack.size() == 0)
    return Executor::ErrCode::StackEmpty;
  /// Check is the top entry a frame.
  if (Stack.back().index() != 0)
    return Executor::ErrCode::StackWrongEntry;
  /// Get the pointer of frame entry.
  Frame = std::move(std::get<0>(Stack.back()));
  FrameIdx.pop_back();
  return Executor::ErrCode::Success;
}

/// Pop and return the label entry. See "include/executor/stackmgr.h".
Executor::ErrCode StackMgr::pop(std::unique_ptr<LabelEntry> &Label) {
  /// Check the size of stack.
  if (Stack.size() == 0)
    return Executor::ErrCode::StackEmpty;
  /// Check is the top entry a label.
  if (Stack.back().index() != 1)
    return Executor::ErrCode::StackWrongEntry;
  /// Get the pointer of label entry.
  Label = std::move(std::get<1>(Stack.back()));
  LabelIdx.pop_back();
  return Executor::ErrCode::Success;
}

/// Pop and return the value entry. See "include/executor/stackmgr.h".
Executor::ErrCode StackMgr::pop(std::unique_ptr<ValueEntry> &Value) {
  /// Check the size of stack.
  if (Stack.size() == 0)
    return Executor::ErrCode::StackEmpty;
  /// Check is the top entry a value.
  if (Stack.back().index() != 2)
    return Executor::ErrCode::StackWrongEntry;
  /// Get the pointer of value entry.
  Value = std::move(std::get<2>(Stack.back()));
  return Executor::ErrCode::Success;
}

/// Drop the top entry of stack. See "include/executor/stackmgr.h".
Executor::ErrCode StackMgr::drop() {
  /// Check the size of stack.
  if (Stack.size() == 0)
    return Executor::ErrCode::StackEmpty;
  /// Check is the top entry's type.
  switch (Stack.back().index()) {
  case 0: /// Frame entry
    FrameIdx.pop_back();
    break;
  case 1: /// Label entry
    LabelIdx.pop_back();
    break;
  default:
    break;
  }
  /// Drop the top entry.
  Stack.pop_back();
  return Executor::ErrCode::Success;
}

/// Frame related operation. See "include/executor/stackmgr.h".
Executor::ErrCode StackMgr::getCurrentFrame(FrameEntry *&Frame) {
  /// Check is there current frame.
  if (FrameIdx.size() == 0)
    return Executor::ErrCode::WrongInstanceAddress;
  /// Get the current frame pointer.
  Frame = std::get<0>(Stack[FrameIdx.back()]).get();
  return Executor::ErrCode::Success;
}

/// Label related operation. See "include/executor/stackmgr.h".
Executor::ErrCode StackMgr::getLabelWithCount(LabelEntry *&Label,
                                              unsigned int Count = 0) {
  /// Check is there at least count + 1 labels.
  if (LabelIdx.size() < Count + 1)
    return Executor::ErrCode::WrongInstanceAddress;
  /// Get the (count + 1)-th top of label.
  unsigned int Idx = LabelIdx.size() - Count - 1;
  Label = std::get<1>(Stack[LabelIdx[Idx]]).get();
  return Executor::ErrCode::Success;
}