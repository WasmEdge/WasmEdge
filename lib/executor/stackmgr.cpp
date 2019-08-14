#include "executor/stackmgr.h"
#include <type_traits>

namespace SSVM {
namespace Executor {

/// Getter of top entry. See "include/executor/stackmgr.h".
template <typename T> Executor::ErrCode StackManager::getTop(T *&Entry) {
  /// Check the size of stack.
  if (Stack.size() == 0)
    return Executor::ErrCode::StackEmpty;
  /// Check is the top entry type matched and get pointer.
  try {
    Entry = std::get<T>(Stack.back()).get();
  } catch (std::bad_variant_access E) {
    return Executor::ErrCode::StackWrongEntry;
  }
  return Executor::ErrCode::Success;
}

/// Push a new frame entry to stack. See "include/executor/stackmgr.h".
template <typename T>
Executor::ErrCode StackManager::push(std::unique_ptr<T> &NewEntry) {
  Stack.push_back(std::move(NewEntry));
  /// If is frame or label, record the index in stack.
  if (std::is_same<T, FrameEntry>::value)
    FrameIdx.push_back(Stack.size() - 1);
  if (std::is_same<T, LabelEntry>::value)
    LabelIdx.push_back(Stack.size() - 1);
  return Executor::ErrCode::Success;
}

/// Pop and return the frame entry. See "include/executor/stackmgr.h".
template <typename T>
Executor::ErrCode StackManager::pop(std::unique_ptr<T> &Entry) {
  /// Check the size of stack.
  if (Stack.size() == 0)
    return Executor::ErrCode::StackEmpty;

  /// Check is the top entry type matched and move pointer.
  try {
    Entry = std::move(std::get<T>(Stack.back()));
  } catch (std::bad_variant_access E) {
    return Executor::ErrCode::StackWrongEntry;
  }

  /// Delete the index of this popped entry.
  if (std::is_same<T, FrameEntry>::value)
    FrameIdx.pop_back();
  if (std::is_same<T, LabelEntry>::value)
    LabelIdx.pop_back();
  /// Drop the top entry.
  Stack.pop_back();
  return Executor::ErrCode::Success;
}

/// Drop the top entry of stack. See "include/executor/stackmgr.h".
Executor::ErrCode StackManager::pop() {
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
Executor::ErrCode StackManager::getCurrentFrame(FrameEntry *&Frame) {
  /// Check is there current frame.
  if (FrameIdx.size() == 0)
    return Executor::ErrCode::WrongInstanceAddress;
  /// Get the current frame pointer.
  Frame = std::get<0>(Stack[FrameIdx.back()]).get();
  return Executor::ErrCode::Success;
}

/// Label related operation. See "include/executor/stackmgr.h".
Executor::ErrCode StackManager::getLabelWithCount(LabelEntry *&Label,
                                                  unsigned int Count = 0) {
  /// Check is there at least count + 1 labels.
  if (LabelIdx.size() < Count + 1)
    return Executor::ErrCode::WrongInstanceAddress;
  /// Get the (count + 1)-th top of label.
  unsigned int Idx = LabelIdx.size() - Count - 1;
  Label = std::get<1>(Stack[LabelIdx[Idx]]).get();
  return Executor::ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
