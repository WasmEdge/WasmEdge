// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/spare_enum_map.h - mapping spare enum to data -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains a class for mapping spare enum value to data.
///
//===----------------------------------------------------------------------===//

#pragma once

#include <algorithm>
#include <array>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace WasmEdge {

template <std::size_t Size, class Key, class T = std::string_view>
class SpareEnumMap {
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

  constexpr SpareEnumMap() noexcept = delete;
  constexpr SpareEnumMap(const SpareEnumMap &) noexcept = delete;
  constexpr SpareEnumMap(SpareEnumMap &&) noexcept = default;
  constexpr SpareEnumMap &operator=(const SpareEnumMap &) noexcept = delete;
  constexpr SpareEnumMap &operator=(SpareEnumMap &&) noexcept = default;

  constexpr SpareEnumMap(value_type (&Array)[Size]) noexcept {
    for (size_t I = 0; I < Size; ++I) {
      size_t J = I;
      for (; J > 0; --J) {
        if (std::less<>()(Data[J - 1], Array[I])) {
          break;
        }
        Data[J].first = std::move(Data[J - 1].first);
        Data[J].second = std::move(Data[J - 1].second);
      }
      Data[J].first = std::move(Array[I].first);
      Data[J].second = std::move(Array[I].second);
    }
  }

  constexpr const_iterator begin() const noexcept { return {Data, 0}; }

  constexpr const_iterator end() const noexcept { return {Data, Size}; }

  constexpr const_iterator find(key_type K) const noexcept {
    if (auto Iter = std::lower_bound(begin(), end(), value_type(K, {}));
        std::equal_to<>()(Iter->first, K)) {
      return Iter;
    }
    return end();
  }

  constexpr const mapped_type &operator[](key_type K) const noexcept {
    return find(K)->second;
  }

private:
  std::array<std::pair<Key, T>, Size + 1> Data;
};

template <class Key, std::size_t Size>
SpareEnumMap(const std::pair<Key, std::string_view> (&)[Size])
    -> SpareEnumMap<Size, Key>;

template <std::size_t Size, class Key, class T>
class SpareEnumMap<Size, Key, T>::ConstIterator {
public:
  using difference_type = SpareEnumMap<Size, Key, T>::difference_type;
  using value_type = SpareEnumMap<Size, Key, T>::value_type;
  using pointer = SpareEnumMap<Size, Key, T>::pointer;
  using reference = SpareEnumMap<Size, Key, T>::reference;
  using iterator_category = std::random_access_iterator_tag;

  constexpr ConstIterator() noexcept = default;
  constexpr ConstIterator(const ConstIterator &) noexcept = default;
  constexpr ConstIterator(ConstIterator &&) noexcept = default;
  constexpr ConstIterator &operator=(const ConstIterator &) noexcept = default;
  constexpr ConstIterator &operator=(ConstIterator &&) noexcept = default;

  constexpr ConstIterator(const std::array<std::pair<Key, T>, Size + 1> &D,
                          size_type I) noexcept
      : Data(std::addressof(D)), Index(I) {}

  constexpr reference operator*() noexcept { return (*Data)[Index]; }
  constexpr const_reference operator*() const noexcept {
    return (*Data)[Index];
  }

  constexpr pointer operator->() noexcept {
    return std::addressof((*Data)[Index]);
  }
  constexpr const_pointer operator->() const noexcept {
    return std::addressof((*Data)[Index]);
  }

  constexpr ConstIterator &operator++() noexcept {
    ++Index;
    return *this;
  }

  constexpr ConstIterator &operator--() noexcept {
    --Index;
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
    Index = static_cast<size_type>(static_cast<difference_type>(Index) + N);
    return *this;
  }

  constexpr ConstIterator &operator-=(difference_type N) noexcept {
    Index = static_cast<size_type>(static_cast<difference_type>(Index) - N);
    return *this;
  }

  friend constexpr ConstIterator operator+(const ConstIterator &LHS,
                                           difference_type RHS) noexcept {
    ConstIterator Iter = LHS;
    return Iter += RHS;
  }

  friend constexpr difference_type
  operator-(const ConstIterator &LHS, const ConstIterator &RHS) noexcept {
    const std::pair<Key, T> *const L = std::addressof((*LHS.Data)[LHS.Index]);
    const std::pair<Key, T> *const R = std::addressof((*RHS.Data)[RHS.Index]);
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
  const std::array<std::pair<Key, T>, Size + 1> *Data = nullptr;
  size_type Index = 0;
};

} // namespace WasmEdge
