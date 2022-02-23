// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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
  StackMgr.push<TOut>(static_cast<TOut>(StackMgr.pop<TIn>()));
  return {};
}

template <typename TIn, typename TOut>
TypeFI<TIn, TOut>
Executor::runTruncateOp(Runtime::StackManager &StackMgr,
                        const AST::Instruction &Instr) const noexcept {
  TIn Z = StackMgr.pop<TIn>();
  // If z is a NaN or an infinity, then the result is undefined.
  if (std::isnan(Z)) {
    spdlog::error(ErrCode::InvalidConvToInt);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Z}, {ValTypeFromType<TIn>()}));
    return Unexpect(ErrCode::InvalidConvToInt);
  }
  if (std::isinf(Z)) {
    spdlog::error(ErrCode::IntegerOverflow);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {Z}, {ValTypeFromType<TIn>()}));
    return Unexpect(ErrCode::IntegerOverflow);
  }
  // If trunc(z) is out of range of target type, then the result is undefined.
  Z = std::trunc(Z);
  const TIn ValTOutMin = static_cast<TIn>(std::numeric_limits<TOut>::min());
  const TIn ValTOutMax = static_cast<TIn>(std::numeric_limits<TOut>::max());
  if (sizeof(TIn) > sizeof(TOut)) {
    // Floating precision is better than integer case.
    if (Z < ValTOutMin || Z > ValTOutMax) {
      spdlog::error(ErrCode::IntegerOverflow);
      spdlog::error(ErrInfo::InfoInstruction(
          Instr.getOpCode(), Instr.getOffset(), {Z}, {ValTypeFromType<TIn>()}));
      return Unexpect(ErrCode::IntegerOverflow);
    }
  } else {
    // Floating precision is worse than integer case.
    if (Z < ValTOutMin || Z >= ValTOutMax) {
      spdlog::error(ErrCode::IntegerOverflow);
      spdlog::error(ErrInfo::InfoInstruction(
          Instr.getOpCode(), Instr.getOffset(), {Z}, {ValTypeFromType<TIn>()}));
      return Unexpect(ErrCode::IntegerOverflow);
    }
  }
  // Else, return trunc(z). Signed case handled.
  StackMgr.push<TOut>(static_cast<TOut>(Z));
  return {};
}

template <typename TIn, typename TOut>
TypeFI<TIn, TOut>
Executor::runTruncateSatOp(Runtime::StackManager &StackMgr) const noexcept {
  TIn Z = StackMgr.pop<TIn>();
  TOut R;
  if (std::isnan(Z)) {
    // If z is a NaN, return 0.
    R = static_cast<TOut>(0);
  } else if (std::isinf(Z)) {
    if (Z < std::numeric_limits<TIn>::lowest()) {
      // If z is -inf, return min limit.
      R = std::numeric_limits<TOut>::min();
    } else {
      // If z is +inf, return max limit.
      R = std::numeric_limits<TOut>::max();
    }
  } else {
    Z = std::trunc(Z);
    TIn ValTOutMin = static_cast<TIn>(std::numeric_limits<TOut>::min());
    TIn ValTOutMax = static_cast<TIn>(std::numeric_limits<TOut>::max());
    if constexpr (sizeof(TIn) > sizeof(TOut)) {
      // Floating precision is better than integer case.
      if (Z < ValTOutMin) {
        R = std::numeric_limits<TOut>::min();
      } else if (Z > ValTOutMax) {
        R = std::numeric_limits<TOut>::max();
      } else {
        R = static_cast<TOut>(Z);
      }
    } else {
      // Floating precision is worse than integer case.
      if (Z < ValTOutMin) {
        R = std::numeric_limits<TOut>::min();
      } else if (Z >= ValTOutMax) {
        R = std::numeric_limits<TOut>::max();
      } else {
        R = static_cast<TOut>(Z);
      }
    }
  }
  StackMgr.push<TOut>(R);
  return {};
}

template <typename TIn, typename TOut, size_t B>
TypeIU<TIn, TOut>
Executor::runExtendOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return i extend to TOut. Signed case handled.
  const TIn V = StackMgr.pop<TIn>();
  TOut R;
  if constexpr (B == sizeof(TIn) * 8) {
    R = static_cast<TOut>(V);
  } else {
    R = static_cast<TOut>(static_cast<TypeFromBytesT<TIn, B>>(V));
  }
  StackMgr.push<TOut>(R);
  return {};
}

template <typename TIn, typename TOut>
TypeIF<TIn, TOut>
Executor::runConvertOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return i convert to TOut. Signed case handled.
  StackMgr.push<TOut>(static_cast<TOut>(StackMgr.pop<TIn>()));
  return {};
}

template <typename TIn, typename TOut>
TypeFF<TIn, TOut>
Executor::runDemoteOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return i convert to TOut. (NaN, inf, and zeros handled)
  StackMgr.push<TOut>(static_cast<TOut>(StackMgr.pop<TIn>()));
  return {};
}

template <typename TIn, typename TOut>
TypeFF<TIn, TOut>
Executor::runPromoteOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return i convert to TOut. (NaN, inf, and zeros handled)
  StackMgr.push<TOut>(static_cast<TOut>(StackMgr.pop<TIn>()));
  return {};
}

template <typename TIn, typename TOut>
TypeNN<TIn, TOut>
Executor::runReinterpretOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return ValVariant with type TOut which copy bits of V.
  const TIn VIn = StackMgr.pop<TIn>();
  TOut VOut;
  std::memcpy(&VOut, &VIn, sizeof(TIn));
  StackMgr.push<TOut>(VOut);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
