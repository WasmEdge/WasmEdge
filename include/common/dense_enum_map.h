// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/dense_enum_map.h - mapping dense enum to data -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains a class for mapping enum value to data.
///
//===----------------------------------------------------------------------===//

#pragma once

#include <array>
#include <memory>
#include <string_view>
#include <type_traits>

namespace WasmEdge {

template <std::size_t Size, class Key, class T = std::string_view>
class DenseEnumMap {
  static_assert(std::is_enum_v<Key>, "Key should be an enum type!");

public:
  class ConstIterator;
  using key_type = Key;
  using mapped_type = T;
  using value_type = const std::pair<Key, T>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using iterator = ConstIterator;
  using const_iterator = ConstIterator;

  constexpr DenseEnumMap() noexcept = delete;
  constexpr DenseEnumMap(const DenseEnumMap &) noexcept = delete;
  constexpr DenseEnumMap(DenseEnumMap &&) noexcept = default;
  constexpr DenseEnumMap &operator=(const DenseEnumMap &) noexcept = delete;
  constexpr DenseEnumMap &operator=(DenseEnumMap &&) noexcept = default;

  constexpr DenseEnumMap(
      const std::pair<Key, std::string_view> (&Array)[Size]) noexcept {
    for (size_type I = 0; I < Size; ++I) {
      Data[static_cast<size_type>(Array[I].first)] = Array[I].second;
    }
  }

  constexpr const mapped_type &operator[](key_type K) const noexcept {
    return Data[static_cast<size_type>(K)];
  }

  constexpr const_iterator begin() const noexcept {
    return {Data, static_cast<size_type>(0)};
  }

  constexpr const_iterator end() const noexcept {
    return {Data, static_cast<size_type>(Size)};
  }

  constexpr const_iterator find(key_type K) const noexcept {
    return {Data, std::min(static_cast<size_type>(K), Size)};
  }

private:
  std::array<T, Size> Data;
};

template <class Key, std::size_t Size>
DenseEnumMap(const std::pair<Key, std::string_view> (&)[Size])
    -> DenseEnumMap<Size, Key>;

template <std::size_t Size, class Key, class T>
class DenseEnumMap<Size, Key, T>::ConstIterator {
public:
  using difference_type = DenseEnumMap<Size, Key, T>::difference_type;
  using value_type = DenseEnumMap<Size, Key, T>::value_type;
  using pointer = DenseEnumMap<Size, Key, T>::pointer;
  using reference = DenseEnumMap<Size, Key, T>::reference;
  using iterator_category = std::random_access_iterator_tag;

  constexpr ConstIterator() noexcept = default;
  constexpr ConstIterator(const ConstIterator &) noexcept = default;
  constexpr ConstIterator(ConstIterator &&) noexcept = default;
  constexpr ConstIterator &operator=(const ConstIterator &) noexcept = default;
  constexpr ConstIterator &operator=(ConstIterator &&) noexcept = default;

  constexpr ConstIterator(const std::array<T, Size> &D, size_type I) noexcept
      : Data(std::addressof(D)),
        Value(static_cast<Key>(I), I < Size ? D[I] : T{}) {}

  constexpr reference operator*() noexcept { return Value; }
  constexpr const_reference operator*() const noexcept { return Value; }

  constexpr pointer operator->() noexcept { return std::addressof(Value); }
  constexpr const_pointer operator->() const noexcept {
    return std::addressof(Value);
  }

  constexpr ConstIterator &operator++() noexcept {
    size_type I = static_cast<size_type>(Value.first);
    ++I;
    Value = {static_cast<Key>(I), I < Size ? (*Data)[I] : T{}};
    return *this;
  }

  constexpr ConstIterator &operator--() noexcept {
    size_type I = static_cast<size_type>(Value.first);
    --I;
    Value = {static_cast<Key>(I), I < Size ? (*Data)[I] : T{}};
    return *this;
  }

  constexpr ConstIterator operator++(int) noexcept {
    ConstIterator Iter(*this);
    ++*this;
    return Iter;
  }

  constexpr ConstIterator operator--(int) noexcept {
    ConstIterator Iter(*this);
    --*this;
    return Iter;
  }

  constexpr ConstIterator &operator+=(difference_type N) noexcept {
    size_type I = static_cast<size_type>(Value.first);
    I = static_cast<size_type>(static_cast<difference_type>(I) + N);
    Value = {static_cast<Key>(I), I < Size ? (*Data)[I] : T{}};
    return *this;
  }

  constexpr ConstIterator &operator-=(difference_type N) noexcept {
    size_type I = static_cast<size_type>(Value.first);
    I = static_cast<size_type>(static_cast<difference_type>(I) - N);
    Value = {static_cast<Key>(I), I < Size ? (*Data)[I] : T{}};
    return *this;
  }

  friend constexpr ConstIterator operator+(const ConstIterator &LHS,
                                           difference_type RHS) noexcept {
    ConstIterator Iter = LHS;
    return Iter += RHS;
  }

  friend constexpr difference_type
  operator-(const ConstIterator &LHS, const ConstIterator &RHS) noexcept {
    const T *const L =
        std::addressof((*LHS.Data)[static_cast<size_type>(LHS.Value.first)]);
    const T *const R =
        std::addressof((*RHS.Data)[static_cast<size_type>(RHS.Value.first)]);
    return L - R;
  }

  constexpr reference operator[](difference_type N) noexcept {
    return *((*this) + N);
  }

  friend constexpr bool operator==(const ConstIterator &LHS,
                                   const ConstIterator &RHS) noexcept {
    return (LHS - RHS) == 0;
  }

  friend constexpr bool operator!=(const ConstIterator &LHS,
                                   const ConstIterator &RHS) noexcept {
    return !(LHS == RHS);
  }

  friend constexpr bool operator<(const ConstIterator &LHS,
                                  const ConstIterator &RHS) noexcept {
    return (RHS - LHS) > 0;
  }

  friend constexpr bool operator>(const ConstIterator &LHS,
                                  const ConstIterator &RHS) noexcept {
    return (LHS - RHS) > 0;
  }

  friend constexpr bool operator<=(const ConstIterator &LHS,
                                   const ConstIterator &RHS) noexcept {
    return !(LHS > RHS);
  }

  friend constexpr bool operator>=(const ConstIterator &LHS,
                                   const ConstIterator &RHS) noexcept {
    return !(LHS < RHS);
  }

private:
  const std::array<T, Size> *Data = nullptr;
  std::pair<Key, T> Value;
};

} // namespace WasmEdge
