#pragma once

#include <type_traits>

namespace SSVM {
namespace Support {

template <typename X, typename Y> inline bool isa(const Y ptr) {
  return dynamic_cast<const X *>(ptr) != nullptr;
}

/// Return true if T is uint32_t or uint64_t.
template <typename T> struct IsWasmUnsign {
  static constexpr const bool value =
      (sizeof(T) == 4 || sizeof(T) == 8) && std::is_unsigned_v<T>;
};

/// Return true if T is int32_t or int64_t.
template <typename T> struct IsWasmSign {
  static constexpr const bool value =
      (sizeof(T) == 4 || sizeof(T) == 8) && std::is_signed_v<T>;
};

/// Return true if T is int32_t, uint32_t, int64_t, or uint64_t.
template <typename T> struct IsWasmInt {
  static constexpr const bool value =
      (sizeof(T) == 4 || sizeof(T) == 8) && !std::is_floating_point_v<T>;
};

/// Return true if T is float or double.
template <typename T> struct IsWasmFloat {
  static constexpr const bool value =
      (sizeof(T) == 4 || sizeof(T) == 8) && std::is_floating_point_v<T>;
};

/// Return true if T is int32_t, uint32_t, int64_t, uint64_t, float, or double.
template <typename T> struct IsWasmType {
  static constexpr const bool value =
      (sizeof(T) == 4 || sizeof(T) == 8) && std::is_arithmetic_v<T>;
};

/// Return true if T is uint32_t, uint64_t, float, or double.
template <typename T> struct IsWasmBuiltIn {
  static constexpr const bool value =
      (sizeof(T) == 4 || sizeof(T) == 8) &&
      (std::is_floating_point_v<T> || std::is_unsigned_v<T>);
};

template <typename T>
typename std::enable_if_t<IsWasmUnsign<T>::value, std::make_signed_t<T>>
toSigned(T Int) {
  return static_cast<std::make_signed_t<T>>(Int);
}

} // namespace Support
} // namespace SSVM
