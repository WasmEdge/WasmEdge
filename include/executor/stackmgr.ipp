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
    Entry = &std::get<T>(Stack.back());
  } catch (std::bad_variant_access E) {
    return ErrCode::StackWrongEntry;
  }
  return ErrCode::Success;
}

/// Push a new frame entry to stack. See "include/executor/stackmgr.h".
template <typename T> TypeE<T, ErrCode> StackManager::push(T &&NewEntry) {
  Stack.push_back(std::forward<T>(NewEntry));
  /// If is frame or label, record the index in stack.
  if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                               Frame>)
    FrameIdx.push_back(Stack.size() - 1);
  if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                               Label>)
    LabelIdx.push_back(Stack.size() - 1);
  return ErrCode::Success;
}

/// Pop and return the frame entry. See "include/executor/stackmgr.h".
template <typename T> TypeE<T, ErrCode> StackManager::pop(T &Entry) {
  /// Check the size of stack.
  if (Stack.size() == 0)
    return ErrCode::StackEmpty;

  /// Check is the top entry type matched and move pointer.
  try {
    Entry = std::move(std::get<T>(Stack.back()));
  } catch (std::bad_variant_access &E) {
    return ErrCode::StackWrongEntry;
  }

  /// Delete the index of this popped entry.
  if constexpr (std::is_same_v<T, Frame>)
    FrameIdx.pop_back();
  if constexpr (std::is_same_v<T, Label>)
    LabelIdx.pop_back();
  /// Drop the top entry.
  Stack.pop_back();
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
