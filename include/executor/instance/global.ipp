// SPDX-License-Identifier: Apache-2.0
#include "executor/instance/global.h"

#include <type_traits>

namespace SSVM {
namespace Executor {
namespace Instance {

/// Getter of value. See "include/executor/instance/global.h".
template <typename T> TypeB<T, ErrCode> GlobalInstance::getValue(T &Val) const {
  /// Get value.
  if (auto Ptr = std::get_if<T>(&Value)) {
    Val = *Ptr;
  } else {
    return ErrCode::TypeNotMatch;
  }
  return ErrCode::Success;
}

/// Setter of value. See "include/executor/instance/global.h".
template <typename T> TypeB<T, ErrCode> GlobalInstance::setValue(const T &Val) {
  if (!std::holds_alternative<T>(Value)) {
    return ErrCode::TypeNotMatch;
  }
  Value = Val;
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
