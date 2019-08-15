#include "executor/valueentry.h"

namespace SSVM {
namespace Executor {

/// Getter for value type. See "include/executor/valueentry.h".
ErrCode ValueEntry::getType(AST::ValType &T) {
  T = Type;
  return ErrCode::Success;
}

/// Value setters
template <typename T> ErrCode ValueEntry::setValue(T &Val) {
  if ((std::is_same<T, int32_t>::value
       && Type == AST::ValType::I32)
      || (std::is_same<T, int64_t>::value
          && Type == AST::ValType::I64)
      || (std::is_same<T, float>::value
          && Type == AST::ValType::F32)
      || (std::is_same<T, double>::value
          && Type == AST::ValType::F64)) {
    Value = Val;
    return ErrCode::Success;
  }
  return ErrCode::TypeNotMatch;
}

template <> ErrCode ValueEntry::setValue<AST::ValVariant>(AST::ValVariant &Val) {
  Value = Val;
  return ErrCode::Success;
}

/// Getter for value. See "include/executor/valueentry.h".
template <typename T> ErrCode ValueEntry::getValue(T &Val) {
  /// Get value.
  try {
    Val = std::get<T>(Value);
  } catch (std::bad_variant_access E) {
    return ErrCode::TypeNotMatch;
  }
  return ErrCode::Success;
}
template <> ErrCode ValueEntry::getValue<AST::ValVariant>(AST::ValVariant &Val) {
  /// Get value
  Val = Value;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
