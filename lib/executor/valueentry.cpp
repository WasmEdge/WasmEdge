#include "executor/valueentry.h"

namespace SSVM {
namespace Executor {

/// Getter for value type. See "include/executor/valueentry.h".
Executor::ErrCode ValueEntry::getType(AST::ValType &T) {
  T = Type;
  return Executor::ErrCode::Success;
}

/// Getter for value. See "include/executor/valueentry.h".
template <typename T> Executor::ErrCode ValueEntry::getValue(T &Val) {
  /// Get value.
  try {
    Val = std::get<T>(Value);
  } catch (std::bad_variant_access E) {
    return Executor::ErrCode::TypeNotMatch;
  }
  return Executor::ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
