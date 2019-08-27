#include "executor/entry/value.h"

namespace SSVM {
namespace Executor {

/// Getter for value. See "include/executor/entry/value.h".
template <typename T> ErrCode ValueEntry::getValue(T &Val) const {
  /// Get value.
  try {
    Val = std::get<T>(Value);
  } catch (std::bad_variant_access E) {
    return ErrCode::TypeNotMatch;
  }
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
