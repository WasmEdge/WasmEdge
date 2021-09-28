// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi/crypto/error.h"
#include "wasi/crypto/api.hpp"

#include <climits>
#include <mutex>
#include <unordered_map>

namespace WasmEdge {
namespace Host {
namespace WASI {
namespace Crypto {
namespace {
// from mozilla-central/mfbt/WrappingOperations.h
template <typename UnsignedType> struct WrapToSignedHelper {
  static_assert(std::is_unsigned_v<UnsignedType>,
                "WrapToSigned must be passed an unsigned type");

  using SignedType = std::make_signed_t<UnsignedType>;

  static constexpr SignedType MaxValue =
      (UnsignedType(1) << (CHAR_BIT * sizeof(SignedType) - 1)) - 1;
  static constexpr SignedType MinValue = -MaxValue - 1;

  static constexpr UnsignedType MinValueUnsigned =
      static_cast<UnsignedType>(MinValue);
  static constexpr UnsignedType MaxValueUnsigned =
      static_cast<UnsignedType>(MaxValue);

  // Overflow-correctness was proven in bug 1432646 and is explained in the
  // comment below.  This function is very hot, both at compile time and
  // runtime, so disable all overflow checking in it.

  static constexpr SignedType compute(UnsignedType aValue) {
    // This algorithm was originally provided here:
    // https://stackoverflow.com/questions/13150449/efficient-unsigned-to-signed-cast-avoiding-implementation-defined-behavior
    //
    // If the value is in the non-negative signed range, just cast.
    //
    // If the value will be negative, compute its delta from the first number
    // past the max signed integer, then add that to the minimum signed value.
    //
    // At the low end: if |u| is the maximum signed value plus one, then it has
    // the same mathematical value as |MinValue| cast to unsigned form.  The
    // delta is zero, so the signed form of |u| is |MinValue| -- exactly the
    // result of adding zero delta to |MinValue|.
    //
    // At the high end: if |u| is the maximum *unsigned* value, then it has all
    // bits set.  |MinValue| cast to unsigned form is purely the high bit set.
    // So the delta is all bits but high set -- exactly |MaxValue|.  And as
    // |MinValue = -MaxValue - 1|, we have |MaxValue + (-MaxValue - 1)| to
    // equal -1.
    //
    // Thus the delta below is in signed range, the corresponding cast is safe,
    // and this computation produces values spanning [MinValue, 0): exactly the
    // desired range of all negative signed integers.
    return (aValue <= MaxValueUnsigned)
               ? static_cast<SignedType>(aValue)
               : static_cast<SignedType>(aValue - MinValueUnsigned) + MinValue;
  }
};
template <typename UnsignedType>
constexpr typename WrapToSignedHelper<UnsignedType>::SignedType
WrapToSigned(UnsignedType aValue) {
  return WrapToSignedHelper<UnsignedType>::compute(aValue);
}
template <typename T> constexpr T ToResult(std::make_unsigned_t<T> aUnsigned) {
  // We could *always* return WrapToSigned and rely on unsigned conversion to
  // undo the wrapping when |T| is unsigned, but this seems clearer.
  return std::is_signed_v<T> ? WrapToSigned(aUnsigned) : aUnsigned;
}

template <typename T> struct WrappingAddHelper {
private:
  using UnsignedT = std::make_unsigned_t<T>;

public:
  static constexpr T compute(T aX, T aY) {
    return ToResult<T>(static_cast<UnsignedT>(aX) + static_cast<UnsignedT>(aY));
  }
};
template <typename T> constexpr T WrappingAdd(T aX, T aY) {
  return WrappingAddHelper<T>::compute(aX, aY);
}

// from MSVC STL
template <class T, std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
[[nodiscard]] constexpr T rotr(T _Val, int _Rotation) noexcept;

template <class T, std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
[[nodiscard]] constexpr T rotl(const T Val, const int Rotation) noexcept {
  constexpr auto Digits = std::numeric_limits<T>::digits;
  const auto Remainder = Rotation % Digits;
  if (Remainder > 0) {
    return static_cast<T>(static_cast<T>(Val << Remainder) |
                          static_cast<T>(Val >> (Digits - Remainder)));
  } else if (Remainder == 0) {
    return Val;
  } else { // Remainder < 0
    return rotr(Val, -Remainder);
  }
}

template <class T, std::enable_if_t<std::is_unsigned_v<T>, int> Enabled>
[[nodiscard]] constexpr T rotr(const T Val, const int Rotation) noexcept {
  constexpr auto Digits = std::numeric_limits<T>::digits;
  const auto Remainder = Rotation % Digits;
  if (Remainder > 0) {
    return static_cast<T>(static_cast<T>(Val >> Remainder) |
                          static_cast<T>(Val << (Digits - Remainder)));
  } else if (Remainder == 0) {
    return Val;
  } else { // Remainder < 0
    return rotl(Val, -Remainder);
  }
}

} // namespace

using Handle = uint32_t;

// HandleType: Clone + Send + Sync(rust)
template <typename HandleType> class HandlesManger {
public:
  HandlesManger(const HandlesManger &) = delete;
  HandlesManger &operator=(const HandlesManger &) = delete;
  HandlesManger(HandlesManger &&) = default;
  HandlesManger &operator=(HandlesManger &&) = default;

  HandlesManger(uint8_t TypeId)
      : LastHandle{rotr(static_cast<Handle>(TypeId), 8)}, TypeId{TypeId} {}

  WasiCryptoExpect<void> close(Handle InputHandle) {
    std::scoped_lock Guard{Mutex};
    if (!Map.erase(InputHandle))
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_CLOSED);
    return {};
  }

  WasiCryptoExpect<Handle> registerModule(HandleType InputType) {
    std::scoped_lock Guard{Mutex};
    auto NextHandle = nextHandle(LastHandle);

    while (true) {
      if (Map.find(NextHandle) == Map.end()) {
        break;
      }
      if (NextHandle == LastHandle) {
        return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_TOO_MANY_HANDLES);
      }
      NextHandle = nextHandle(LastHandle);
    }
    LastHandle = NextHandle;
    if (!Map.insert({NextHandle, InputType}).second) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INTERNAL_ERROR);
    }
    return NextHandle;
  }

  WasiCryptoExpect<HandleType> get(Handle InputHandle) {
    std::scoped_lock Guard{Mutex};
    auto HandleValue = Map.find(InputHandle);
    if (HandleValue == Map.end()) {
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_HANDLE);
    }
    return HandleValue->second;
  }

private:
  Handle nextHandle(Handle InputHandle) {
    auto AddedValue = WrappingAdd(InputHandle, 1u) << 8;
    return rotr(AddedValue | static_cast<Handle>(TypeId), 8);
  }

  std::mutex Mutex;
  Handle LastHandle;
  std::unordered_map<Handle, HandleType> Map;
  uint8_t TypeId;
};

} // namespace Crypto
} // namespace WASI
} // namespace Host
} // namespace WasmEdge