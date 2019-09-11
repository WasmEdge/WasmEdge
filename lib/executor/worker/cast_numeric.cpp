#include "executor/common.h"
#include "executor/entry/value.h"
#include "executor/worker.h"
#include "executor/worker/util.h"
#include <cmath>

namespace SSVM {
namespace Executor {

template <typename TIn, typename TOut>
ErrCode Worker::runWrapOp(const ValueEntry *Val) {
  TIn I = retrieveValue<TIn>(*Val);
  return StackMgr.pushValue(static_cast<TOut>(I));
}

template <typename TIn, typename TOut>
ErrCode Worker::runTruncSOp(const ValueEntry *Val) {
  TIn Z = retrieveValue<TIn>(*Val);
  /// If z is a NaN or an infinity, then the result is undefined.
  if (std::isnan(Z) || std::isinf(Z)) {
    return ErrCode::CastingError;
  }
  /// If trunc(z) is out of range of target type, then the result is undefined.
  Z = std::trunc(Z);
  TIn SignedTOutMin =
      static_cast<TIn>(std::numeric_limits<std::make_signed_t<TOut>>::min());
  TIn SignedTOutMax =
      static_cast<TIn>(std::numeric_limits<std::make_signed_t<TOut>>::max());
  if (Z <= SignedTOutMin - 1.0 || Z >= SignedTOutMax + 1.0) {
    return ErrCode::CastingError;
  }
  /// Else, return trunc(z).
  std::make_signed_t<TOut> Res = Z;
  return StackMgr.pushValue(static_cast<TOut>(Res));
}

template <typename TIn, typename TOut>
ErrCode Worker::runTruncUOp(const ValueEntry *Val) {
  TIn Z = retrieveValue<TIn>(*Val);
  /// If z is a NaN or an infinity, then the result is undefined.
  if (std::isnan(Z) || std::isinf(Z)) {
    return ErrCode::CastingError;
  }
  /// If trunc(z) is out of range of target type, then the result is undefined.
  Z = std::trunc(Z);
  TIn SignedTOutMax = static_cast<TIn>(std::numeric_limits<TOut>::max());
  if (Z <= -1.0 || Z >= SignedTOutMax + 1.0) {
    return ErrCode::CastingError;
  }
  /// Else, return trunc(z).
  return StackMgr.pushValue(static_cast<TOut>(std::trunc(Z)));
}

template <typename TIn, typename TOut>
ErrCode Worker::runExtendSOp(const ValueEntry *Val) {
  TIn I = retrieveValue<TIn>(*Val);
  /// Return signed(i) extend to TOut.
  return StackMgr.pushValue(static_cast<TOut>(toSigned(I)));
}

template <typename TIn, typename TOut>
ErrCode Worker::runExtendUOp(const ValueEntry *Val) {
  TIn I = retrieveValue<TIn>(*Val);
  /// Return i extend to TOut.
  return StackMgr.pushValue(static_cast<TOut>(I));
}

template <typename TIn, typename TOut>
ErrCode Worker::runConvertSOp(const ValueEntry *Val) {
  TIn I = retrieveValue<TIn>(*Val);
  /// Return signed(i) convert to TOut.
  return StackMgr.pushValue(static_cast<TOut>(toSigned(I)));
}

template <typename TIn, typename TOut>
ErrCode Worker::runConvertUOp(const ValueEntry *Val) {
  TIn I = retrieveValue<TIn>(*Val);
  /// Return i convert to TOut.
  return StackMgr.pushValue(static_cast<TOut>(I));
}

template <typename TIn, typename TOut>
ErrCode Worker::runDemoteOp(const ValueEntry *Val) {
  TIn Z = retrieveValue<TIn>(*Val);
  /// Return i convert to TOut. (NaN, inf, and zeros handled)
  return StackMgr.pushValue(static_cast<TOut>(Z));
}

template <typename TIn, typename TOut>
ErrCode Worker::runPromoteOp(const ValueEntry *Val) {
  TIn Z = retrieveValue<TIn>(*Val);
  /// Return i convert to TOut. (NaN, inf, and zeros handled)
  return StackMgr.pushValue(static_cast<TOut>(Z));
}

template <typename TIn, typename TOut>
ErrCode Worker::runReinterpretOp(const ValueEntry *Val) {
  TIn V = retrieveValue<TIn>(*Val);
  /// Return value with type TOut which copy bits of V.
  TOut Res;
  memcpy(&Res, &V, sizeof(TOut));
  return StackMgr.pushValue(Res);
}

} // namespace Executor
} // namespace SSVM