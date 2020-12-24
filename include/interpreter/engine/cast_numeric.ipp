// SPDX-License-Identifier: Apache-2.0
#include "common/log.h"
#include "common/value.h"
#include "interpreter/interpreter.h"

#include <cmath>
#include <cstring>

namespace SSVM {
namespace Interpreter {

/// Helper for types transform
namespace {
template <typename T, size_t B = sizeof(T)> struct TypeFromBytes {
  using type = T;
};
template <typename T> struct TypeFromBytes<T, 8> {
  using type =
      typename std::conditional<std::is_signed_v<T>, int8_t, uint8_t>::type;
};
template <typename T> struct TypeFromBytes<T, 16> {
  using type =
      typename std::conditional<std::is_signed_v<T>, int16_t, uint16_t>::type;
};
template <typename T> struct TypeFromBytes<T, 32> {
  using type =
      typename std::conditional<std::is_signed_v<T>, int32_t, uint32_t>::type;
};
template <typename T, size_t B>
using TypeFromBytesT = typename TypeFromBytes<T, B>::type;
} // namespace

template <typename TIn, typename TOut>
TypeUU<TIn, TOut> Interpreter::runWrapOp(ValVariant &Val) const {
  Val = static_cast<TOut>(retrieveValue<TIn>(Val));
  return {};
}

template <typename TIn, typename TOut>
TypeFI<TIn, TOut> Interpreter::runTruncateOp(const AST::Instruction &Instr,
                                             ValVariant &Val) const {
  TIn Z = retrieveValue<TIn>(Val);
  /// If z is a NaN or an infinity, then the result is undefined.
  if (std::isnan(Z)) {
    LOG(ERROR) << ErrCode::InvalidConvToInt;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Val}, {ValTypeFromType<TIn>()});
    return Unexpect(ErrCode::InvalidConvToInt);
  }
  if (std::isinf(Z)) {
    LOG(ERROR) << ErrCode::IntegerOverflow;
    LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Val}, {ValTypeFromType<TIn>()});
    return Unexpect(ErrCode::IntegerOverflow);
  }
  /// If trunc(z) is out of range of target type, then the result is undefined.
  Z = std::trunc(Z);
  TIn ValTOutMin = static_cast<TIn>(std::numeric_limits<TOut>::min());
  TIn ValTOutMax = static_cast<TIn>(std::numeric_limits<TOut>::max());
  if (sizeof(TIn) > sizeof(TOut)) {
    /// Floating precision is better than integer case.
    if (Z < ValTOutMin || Z > ValTOutMax) {
      LOG(ERROR) << ErrCode::IntegerOverflow;
      LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                             Instr.getOffset(), {Val},
                                             {ValTypeFromType<TIn>()});
      return Unexpect(ErrCode::IntegerOverflow);
    }
  } else {
    /// Floating precision is worse than integer case.
    if (Z < ValTOutMin || Z >= ValTOutMax) {
      LOG(ERROR) << ErrCode::IntegerOverflow;
      LOG(ERROR) << ErrInfo::InfoInstruction(Instr.getOpCode(),
                                             Instr.getOffset(), {Val},
                                             {ValTypeFromType<TIn>()});
      return Unexpect(ErrCode::IntegerOverflow);
    }
  }
  /// Else, return trunc(z). Signed case handled.
  retrieveValue<TOut>(Val) = Z;
  return {};
}

template <typename TIn, typename TOut>
TypeFI<TIn, TOut> Interpreter::runTruncateSatOp(ValVariant &Val) const {
  TIn Z = retrieveValue<TIn>(Val);
  if (std::isnan(Z)) {
    /// If z is a NaN, return 0.
    retrieveValue<TOut>(Val) = 0;
  } else if (std::isinf(Z)) {
    if (Z < std::numeric_limits<TIn>::lowest()) {
      /// If z is -inf, return min limit.
      retrieveValue<TOut>(Val) = std::numeric_limits<TOut>::min();
    } else {
      /// If z is +inf, return max limit.
      retrieveValue<TOut>(Val) = std::numeric_limits<TOut>::max();
    }
  } else {
    Z = std::trunc(Z);
    TIn ValTOutMin = static_cast<TIn>(std::numeric_limits<TOut>::min());
    TIn ValTOutMax = static_cast<TIn>(std::numeric_limits<TOut>::max());
    if (sizeof(TIn) > sizeof(TOut)) {
      /// Floating precision is better than integer case.
      if (Z < ValTOutMin) {
        retrieveValue<TOut>(Val) = std::numeric_limits<TOut>::min();
      } else if (Z > ValTOutMax) {
        retrieveValue<TOut>(Val) = std::numeric_limits<TOut>::max();
      } else {
        retrieveValue<TOut>(Val) = Z;
      }
    } else {
      /// Floating precision is worse than integer case.
      if (Z < ValTOutMin) {
        retrieveValue<TOut>(Val) = std::numeric_limits<TOut>::min();
      } else if (Z >= ValTOutMax) {
        retrieveValue<TOut>(Val) = std::numeric_limits<TOut>::max();
      } else {
        retrieveValue<TOut>(Val) = Z;
      }
    }
  }
  return {};
}

template <typename TIn, typename TOut, size_t B>
TypeIU<TIn, TOut> Interpreter::runExtendOp(ValVariant &Val) const {
  /// Return i extend to TOut. Signed case handled.
  if (B == sizeof(TIn) * 8) {
    retrieveValue<TOut>(Val) = retrieveValue<TIn>(Val);
  } else {
    retrieveValue<TOut>(Val) =
        static_cast<TypeFromBytesT<TIn, B>>(retrieveValue<TIn>(Val));
  }
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
TypeNN<TIn, TOut> Interpreter::runReinterpretOp(ValVariant &Val) const {
  /// Return ValVariant with type TOut which copy bits of V.
  TOut VOut;
  TIn VIn = retrieveValue<TIn>(Val);
  std::memcpy(&VOut, &VIn, sizeof(TIn));
  retrieveValue<TOut>(Val) = VOut;
  return {};
}

} // namespace Interpreter
} // namespace SSVM
