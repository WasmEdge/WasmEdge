// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include <cmath>
#include <cstring>

namespace WasmEdge {
namespace Executor {

// Helper for types transform
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
TypeUU<TIn, TOut>
Executor::runWrapOp(Runtime::StackManager &StackMgr) const noexcept {
  StackMgr.emplaceTop<TOut>(static_cast<TOut>(StackMgr.peekTop<TIn>()));
  return {};
}

template <typename TIn, typename TOut>
TypeFI<TIn, TOut>
Executor::runTruncateOp(Runtime::StackManager &StackMgr,
                        const AST::Instruction &Instr) const noexcept {
  TIn Val = StackMgr.peekTop<TIn>();
  // If val is a NaN or an infinity, then the result is undefined.
  if (std::isnan(Val)) {
    spdlog::error(ErrCode::Value::InvalidConvToInt);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Val}, {ValTypeFromType<TIn>()}));
    return Unexpect(ErrCode::Value::InvalidConvToInt);
  }
  if (std::isinf(Val)) {
    spdlog::error(ErrCode::Value::IntegerOverflow);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Val}, {ValTypeFromType<TIn>()}));
    return Unexpect(ErrCode::Value::IntegerOverflow);
  }
  // If trunc(val) is out of range of target type, then the result is undefined.
  Val = std::trunc(Val);
  TIn ValTOutMin = static_cast<TIn>(std::numeric_limits<TOut>::min());
  TIn ValTOutMax = static_cast<TIn>(std::numeric_limits<TOut>::max());
  if constexpr (sizeof(TIn) > sizeof(TOut)) {
    // Floating precision is better than integer case.
    if (Val < ValTOutMin || Val > ValTOutMax) {
      spdlog::error(ErrCode::Value::IntegerOverflow);
      spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(),
                                             Instr.getOffset(), {Val},
                                             {ValTypeFromType<TIn>()}));
      return Unexpect(ErrCode::Value::IntegerOverflow);
    }
  } else {
    // Floating precision is worse than integer case.
    if (Val < ValTOutMin || Val >= ValTOutMax) {
      spdlog::error(ErrCode::Value::IntegerOverflow);
      spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(),
                                             Instr.getOffset(), {Val},
                                             {ValTypeFromType<TIn>()}));
      return Unexpect(ErrCode::Value::IntegerOverflow);
    }
  }
  // Else, return trunc(z). Signed case handled.
  StackMgr.emplaceTop(static_cast<TOut>(Val));
  return {};
}

template <typename TIn, typename TOut>
TypeFI<TIn, TOut>
Executor::runTruncateSatOp(Runtime::StackManager &StackMgr) const noexcept {
  TIn Z = StackMgr.peekTop<TIn>();
  if (unlikely(std::isnan(Z))) {
    // If z is a NaN, return 0.
    StackMgr.emplaceTop(static_cast<TOut>(0));
  } else if (unlikely(std::isinf(Z))) {
    if (Z < std::numeric_limits<TIn>::lowest()) {
      // If z is -inf, return min limit.
      StackMgr.emplaceTop(std::numeric_limits<TOut>::min());
    } else {
      // If z is +inf, return max limit.
      StackMgr.emplaceTop(std::numeric_limits<TOut>::max());
    }
  } else {
    Z = std::trunc(Z);
    TIn ValTOutMin = static_cast<TIn>(std::numeric_limits<TOut>::min());
    TIn ValTOutMax = static_cast<TIn>(std::numeric_limits<TOut>::max());
    if constexpr (sizeof(TIn) > sizeof(TOut)) {
      // Floating precision is better than integer case.
      if (Z < ValTOutMin) {
        StackMgr.emplaceTop(std::numeric_limits<TOut>::min());
      } else if (Z > ValTOutMax) {
        StackMgr.emplaceTop(std::numeric_limits<TOut>::max());
      } else {
        StackMgr.emplaceTop(static_cast<TOut>(Z));
      }
    } else {
      // Floating precision is worse than integer case.
      if (Z < ValTOutMin) {
        StackMgr.emplaceTop(std::numeric_limits<TOut>::min());
      } else if (Z >= ValTOutMax) {
        StackMgr.emplaceTop(std::numeric_limits<TOut>::max());
      } else {
        StackMgr.emplaceTop(static_cast<TOut>(Z));
      }
    }
  }
  return {};
}

template <typename TIn, typename TOut, size_t B>
TypeIU<TIn, TOut>
Executor::runExtendOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return i extend to TOut. Signed case handled.
  if constexpr (B == sizeof(TIn) * 8) {
    StackMgr.emplaceTop<TOut>(static_cast<TOut>(StackMgr.peekTop<TIn>()));
  } else {
    StackMgr.emplaceTop<TOut>(static_cast<TOut>(
        static_cast<TypeFromBytesT<TIn, B>>(StackMgr.peekTop<TIn>())));
  }
  return {};
}

template <typename TIn, typename TOut>
TypeIF<TIn, TOut>
Executor::runConvertOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return i convert to TOut. Signed case handled.
  StackMgr.emplaceTop<TOut>(static_cast<TOut>(StackMgr.peekTop<TIn>()));
  return {};
}

template <typename TIn, typename TOut>
TypeFF<TIn, TOut>
Executor::runDemoteOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return i convert to TOut. (NaN, inf, and zeros handled)
  StackMgr.emplaceTop<TOut>(static_cast<TOut>(StackMgr.peekTop<TIn>()));
  return {};
}

template <typename TIn, typename TOut>
TypeFF<TIn, TOut>
Executor::runPromoteOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return i convert to TOut. (NaN, inf, and zeros handled)
  StackMgr.emplaceTop<TOut>(static_cast<TOut>(StackMgr.peekTop<TIn>()));
  return {};
}

template <typename TIn, typename TOut>
TypeNN<TIn, TOut>
Executor::runReinterpretOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return ValVariant with type TOut which copy bits of V.
  TOut VOut;
  TIn VIn = StackMgr.peekTop<TIn>();
  std::memcpy(&VOut, &VIn, sizeof(TIn));
  StackMgr.emplaceTop(std::move(VOut));
  return {};
}

} // namespace Executor
} // namespace WasmEdge
