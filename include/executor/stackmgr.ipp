#include "stackmgr.h"
#include <type_traits>

namespace SSVM {
namespace Executor {

/// Getter of top entry. See "include/executor/stackmgr.h".
template <typename T> TypeE<T, ErrCode> StackManager::getTop(T *&Entry) {
  /// Check the size of stack.
  if (Stack.size() == 0)
    return ErrCode::StackEmpty;
  /// Check is the top entry type matched and get pointer.
  try {
    Entry = std::get<std::unique_ptr<T>>(Stack.back()).get();
  } catch (std::bad_variant_access E) {
    return ErrCode::StackWrongEntry;
  }
  return ErrCode::Success;
}

/// Push a new frame entry to stack. See "include/executor/stackmgr.h".
template <typename T>
TypeE<T, ErrCode> StackManager::push(std::unique_ptr<T> &&NewEntry) {
  Stack.push_back(std::move(NewEntry));
  /// If is frame or label, record the index in stack.
  if (std::is_same<T, FrameEntry>::value)
    FrameIdx.push_back(Stack.size() - 1);
  if (std::is_same<T, LabelEntry>::value)
    LabelIdx.push_back(Stack.size() - 1);
  return ErrCode::Success;
}

/// Pop and return the frame entry. See "include/executor/stackmgr.h".
template <typename T>
TypeE<T, ErrCode> StackManager::pop(std::unique_ptr<T> &Entry) {
  /// Check the size of stack.
  if (Stack.size() == 0)
    return ErrCode::StackEmpty;

  /// Check is the top entry type matched and move pointer.
  try {
    Entry = std::move(std::get<std::unique_ptr<T>>(Stack.back()));
  } catch (std::bad_variant_access E) {
    return ErrCode::StackWrongEntry;
  }

  /// Delete the index of this popped entry.
  if (std::is_same<T, FrameEntry>::value)
    FrameIdx.pop_back();
  if (std::is_same<T, LabelEntry>::value)
    LabelIdx.pop_back();
  /// Drop the top entry.
  Stack.pop_back();
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
