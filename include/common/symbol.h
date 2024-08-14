// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/symbol.h - Symbol definition ----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Symbol, which holds the handle to
/// the loaded shared library.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <memory>

namespace WasmEdge {
class Executable;

/// Holder class for library symbol
template <typename T = void> class Symbol {
private:
  friend class Executable;
  template <typename> friend class Symbol;

  Symbol(std::shared_ptr<const Executable> H, T *S) noexcept
      : Library(std::move(H)), Pointer(S) {}

public:
  Symbol() = default;
  Symbol(const Symbol &) = default;
  Symbol &operator=(const Symbol &) = default;
  Symbol(Symbol &&) = default;
  Symbol &operator=(Symbol &&) = default;

  explicit Symbol(T *S) noexcept : Pointer(S) {}

  operator bool() const noexcept { return Pointer != nullptr; }
  auto &operator*() const noexcept { return *Pointer; }
  auto operator->() const noexcept { return Pointer; }

  template <typename... ArgT>
  auto operator()(ArgT... Args) const
      noexcept(noexcept(this->Pointer(std::forward<ArgT>(Args)...))) {
    return Pointer(std::forward<ArgT>(Args)...);
  }

  auto get() const noexcept { return Pointer; }
  auto deref() & { return Symbol<std::remove_pointer_t<T>>(Library, *Pointer); }
  auto deref() && {
    return Symbol<std::remove_pointer_t<T>>(std::move(Library), *Pointer);
  }

private:
  std::shared_ptr<const Executable> Library;
  T *Pointer = nullptr;
};

template <typename T> class Symbol<T[]> {
private:
  friend class Executable;
  template <typename> friend class Symbol;

  Symbol(std::shared_ptr<const Executable> H, T (*S)[]) noexcept
      : Library(std::move(H)), Pointer(*S) {}

public:
  Symbol() = default;
  Symbol(const Symbol &) = default;
  Symbol &operator=(const Symbol &) = default;
  Symbol(Symbol &&) = default;
  Symbol &operator=(Symbol &&) = default;

  explicit Symbol(T *S) noexcept : Pointer(S) {}

  operator bool() const noexcept { return Pointer != nullptr; }
  auto &operator[](size_t Index) const noexcept { return Pointer[Index]; }

  auto get() const noexcept { return Pointer; }
  auto index(size_t Index) & { return Symbol<T>(Library, &Pointer[Index]); }
  auto index(size_t Index) && {
    return Symbol<T>(std::move(Library), &Pointer[Index]);
  }

private:
  std::shared_ptr<const Executable> Library;
  T *Pointer = nullptr;
};

} // namespace WasmEdge
