// SPDX-License-Identifier: Apache-2.0
#include "common/value.h"
#include "interpreter/interpreter.h"

#include <cmath>
#include <cstring>

namespace SSVM {
namespace Interpreter {

template <typename TIn, typename TOut>
TypeUU<TIn, TOut> Interpreter::runWrapOp(ValVariant &Val) const {
  Val = static_cast<TOut>(retrieveValue<TIn>(Val));
  return {};
}

template <typename TIn, typename TOut>
TypeFI<TIn, TOut> Interpreter::runTruncateOp(ValVariant &Val) const {
  TIn Z = retrieveValue<TIn>(Val);
  /// If z is a NaN or an infinity, then the result is undefined.
  if (std::isnan(Z) || std::isinf(Z)) {
    return Unexpect(ErrCode::CastingError);
  }
  /// If trunc(z) is out of range of target type, then the result is undefined.
  Z = std::trunc(Z);
  TIn ValTOutMin = static_cast<TIn>(std::numeric_limits<TOut>::min());
  TIn ValTOutMax = static_cast<TIn>(std::numeric_limits<TOut>::max());
  if (sizeof(TIn) > sizeof(TOut)) {
    /// Floating precision is better than integer case.
    if (Z < ValTOutMin || Z > ValTOutMax) {
      return Unexpect(ErrCode::CastingError);
    }
  } else {
    /// Floating precision is worse than integer case.
    if (Z < ValTOutMin || Z >= ValTOutMax) {
      return Unexpect(ErrCode::CastingError);
    }
  }
  /// Else, return trunc(z). Signed case handled.
  retrieveValue<TOut>(Val) = Z;
  return {};
}

template <typename TIn, typename TOut>
TypeIU<TIn, TOut> Interpreter::runExtendOp(ValVariant &Val) const {
  /// Return i extend to TOut. Signed case handled.
  retrieveValue<TOut>(Val) = retrieveValue<TIn>(Val);
  return {};
}

template <typename TIn, typename TOut>
TypeIF<TIn, TOut> Interpreter::runConvertOp(ValVariant &Val) const {
  /// Return i convert to TOut. Signed case handled.
  retrieveValue<TOut>(Val) = retrieveValue<TIn>(Val);
  return {};
}

template <typename TIn, typename TOut>
TypeFF<TIn, TOut> Interpreter::runDemoteOp(ValVariant &Val) const {
  /// Return i convert to TOut. (NaN, inf, and zeros handled)
  retrieveValue<TOut>(Val) = retrieveValue<TIn>(Val);
  return {};
}

template <typename TIn, typename TOut>
TypeFF<TIn, TOut> Interpreter::runPromoteOp(ValVariant &Val) const {
  /// Return i convert to TOut. (NaN, inf, and zeros handled)
  retrieveValue<TOut>(Val) = retrieveValue<TIn>(Val);
  return {};
}

template <typename TIn, typename TOut>
TypeBB<TIn, TOut> Interpreter::runReinterpretOp(ValVariant &Val) const {
  /// Return ValVariant with type TOut which copy bits of V.
  TOut VOut;
  TIn VIn = retrieveValue<TIn>(Val);
  std::memcpy(&VOut, &VIn, sizeof(TIn));
  retrieveValue<TOut>(Val) = VOut;
  return {};
}

} // namespace Interpreter
} // namespace SSVM
