// SPDX-License-Identifier: Apache-2.0
#include "executor/common.h"
#include "executor/entry/value.h"
#include "executor/worker.h"
#include "common/value.h"

#include <cmath>
#include <cstring>

namespace SSVM {
namespace Executor {

template <typename TIn, typename TOut>
TypeUU<TIn, TOut, ErrCode> Worker::runWrapOp(Value &Val) const {
  Val = static_cast<TOut>(retrieveValue<TIn>(Val));
  return ErrCode::Success;
}

template <typename TIn, typename TOut>
TypeFI<TIn, TOut, ErrCode> Worker::runTruncateOp(Value &Val) const {
  TIn Z = retrieveValue<TIn>(Val);
  /// If z is a NaN or an infinity, then the result is undefined.
  if (std::isnan(Z) || std::isinf(Z)) {
    return ErrCode::CastingError;
  }
  /// If trunc(z) is out of range of target type, then the result is undefined.
  Z = std::trunc(Z);
  TIn ValTOutMin = static_cast<TIn>(std::numeric_limits<TOut>::min());
  TIn ValTOutMax = static_cast<TIn>(std::numeric_limits<TOut>::max());
  if (Z <= ValTOutMin - 1.0 || Z >= ValTOutMax + 1.0) {
    return ErrCode::CastingError;
  }
  /// Else, return trunc(z). Signed case handled.
  retrieveValue<TOut>(Val) = Z;
  return ErrCode::Success;
}

template <typename TIn, typename TOut>
TypeIU<TIn, TOut, ErrCode> Worker::runExtendOp(Value &Val) const {
  /// Return i extend to TOut. Signed case handled.
  retrieveValue<TOut>(Val) = retrieveValue<TIn>(Val);
  return ErrCode::Success;
}

template <typename TIn, typename TOut>
TypeIF<TIn, TOut, ErrCode> Worker::runConvertOp(Value &Val) const {
  /// Return i convert to TOut. Signed case handled.
  retrieveValue<TOut>(Val) = retrieveValue<TIn>(Val);
  return ErrCode::Success;
}

template <typename TIn, typename TOut>
TypeFF<TIn, TOut, ErrCode> Worker::runDemoteOp(Value &Val) const {
  /// Return i convert to TOut. (NaN, inf, and zeros handled)
  retrieveValue<TOut>(Val) = retrieveValue<TIn>(Val);
  return ErrCode::Success;
}

template <typename TIn, typename TOut>
TypeFF<TIn, TOut, ErrCode> Worker::runPromoteOp(Value &Val) const {
  /// Return i convert to TOut. (NaN, inf, and zeros handled)
  retrieveValue<TOut>(Val) = retrieveValue<TIn>(Val);
  return ErrCode::Success;
}

template <typename TIn, typename TOut>
TypeBB<TIn, TOut, ErrCode> Worker::runReinterpretOp(Value &Val) const {
  /// Return value with type TOut which copy bits of V.
  memcpy(&retrieveValue<TOut>(Val), &retrieveValue<TIn>(Val), sizeof(TOut));
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
