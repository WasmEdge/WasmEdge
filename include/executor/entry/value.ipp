#include "executor/entry/value.h"

namespace SSVM {
namespace Executor {

/// Setter for value. See "include/executor/entry/value.h".
template <typename T> TypeB<T, ErrCode> ValueEntry::setValue(const T &Val) {
  if ((std::is_same<T, uint32_t>::value && Type == AST::ValType::I32) ||
      (std::is_same<T, uint64_t>::value && Type == AST::ValType::I64) ||
      (std::is_same<T, float>::value && Type == AST::ValType::F32) ||
      (std::is_same<T, double>::value && Type == AST::ValType::F64)) {
    Value = Val;
    return ErrCode::Success;
  }
  return ErrCode::TypeNotMatch;
}

/// Getter for value. See "include/executor/entry/value.h".
template <typename T> TypeB<T, ErrCode> ValueEntry::getValue(T &Val) const {
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
