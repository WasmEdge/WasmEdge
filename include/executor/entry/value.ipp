#include "executor/entry/value.h"

namespace SSVM {
namespace Executor {

/// Setter for value. See "include/executor/entry/value.h".
template <typename T> TypeB<T, ErrCode> ValueEntry::setValue(const T &Val) {
  Value = Val;
  return ErrCode::Success;
}

/// Getter for value. See "include/executor/entry/value.h".
template <typename T> TypeB<T, ErrCode> ValueEntry::getValue(T &Val) const {
  /// Get value.
  Val = std::get<T>(Value);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
