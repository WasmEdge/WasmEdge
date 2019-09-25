#include "executor/common.h"
#include "executor/entry/value.h"
#include "executor/worker.h"
#include "executor/worker/util.h"
#include "support/casting.h"
#include <cmath>

namespace SSVM {
namespace Executor {

template <typename TIn, typename TOut>
TypeUU<TIn, TOut, ErrCode> Worker::runWrapOp(const ValueEntry *Val) {
  TIn I = retrieveValue<TIn>(*Val);
  return StackMgr.pushValue(static_cast<TOut>(I));
}

template <typename TIn, typename TOut>
TypeFI<TIn, TOut, ErrCode> Worker::runTruncateOp(const ValueEntry *Val) {
  TIn Z = retrieveValue<TIn>(*Val);
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
  TOut Res = Z;
  return StackMgr.pushValue(static_cast<std::make_unsigned_t<TOut>>(Res));
}

template <typename TIn, typename TOut>
TypeIU<TIn, TOut, ErrCode> Worker::runExtendOp(const ValueEntry *Val) {
  TIn I = retrieveValue<TIn>(*Val);
  /// Return i extend to TOut. Signed case handled.
  return StackMgr.pushValue(static_cast<TOut>(I));
}

template <typename TIn, typename TOut>
TypeIF<TIn, TOut, ErrCode> Worker::runConvertOp(const ValueEntry *Val) {
  TIn I = retrieveValue<TIn>(*Val);
  /// Return i convert to TOut. Signed case handled.
  return StackMgr.pushValue(static_cast<TOut>(I));
}

template <typename TIn, typename TOut>
TypeFF<TIn, TOut, ErrCode> Worker::runDemoteOp(const ValueEntry *Val) {
  TIn Z = retrieveValue<TIn>(*Val);
  /// Return i convert to TOut. (NaN, inf, and zeros handled)
  return StackMgr.pushValue(static_cast<TOut>(Z));
}

template <typename TIn, typename TOut>
TypeFF<TIn, TOut, ErrCode> Worker::runPromoteOp(const ValueEntry *Val) {
  TIn Z = retrieveValue<TIn>(*Val);
  /// Return i convert to TOut. (NaN, inf, and zeros handled)
  return StackMgr.pushValue(static_cast<TOut>(Z));
}

template <typename TIn, typename TOut>
TypeBB<TIn, TOut, ErrCode> Worker::runReinterpretOp(const ValueEntry *Val) {
  TIn V = retrieveValue<TIn>(*Val);
  /// Return value with type TOut which copy bits of V.
  TOut Res;
  memcpy(&Res, &V, sizeof(TOut));
  return StackMgr.pushValue(Res);
}

} // namespace Executor
} // namespace SSVM