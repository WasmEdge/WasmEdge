// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/loader/shared_library.h - Shared library definition ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the SharedLibrary, which holds handle
/// to loaded library.
///
//===----------------------------------------------------------------------===//
#pragma once
#include "common/defines.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include <memory>

#if WASMEDGE_OS_WINDOWS
#include <boost/winapi/dll.hpp>
#endif

namespace WasmEdge {
namespace Loader {

class SharedLibrary;

/// Holder class for library symbol
template <typename T = void> class Symbol {
private:
  friend class SharedLibrary;
  template <typename> friend class Symbol;

  Symbol(std::shared_ptr<SharedLibrary> H, T *S) noexcept
      : Library(std::move(H)), Pointer(S) {}

public:
  Symbol() = default;
  Symbol(const Symbol &) = default;
  Symbol &operator=(const Symbol &) = default;
  Symbol(Symbol &&) = default;
  Symbol &operator=(Symbol &&) = default;

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
  std::shared_ptr<SharedLibrary> Library;
  T *Pointer = nullptr;
};

template <typename T> class Symbol<T[]> {
private:
  friend class SharedLibrary;
  template <typename> friend class Symbol;

  Symbol(std::shared_ptr<SharedLibrary> H, T (*S)[]) noexcept
      : Library(std::move(H)), Pointer(*S) {}

public:
  Symbol() = default;
  Symbol(const Symbol &) = default;
  Symbol &operator=(const Symbol &) = default;
  Symbol(Symbol &&) = default;
  Symbol &operator=(Symbol &&) = default;

  operator bool() const noexcept { return Pointer != nullptr; }
  auto &operator[](size_t Index) const noexcept { return Pointer[Index]; }

  auto get() const noexcept { return Pointer; }
  auto index(size_t Index) & { return Symbol<T>(Library, &Pointer[Index]); }
  auto index(size_t Index) && {
    return Symbol<T>(std::move(Library), &Pointer[Index]);
  }

private:
  std::shared_ptr<SharedLibrary> Library;
  T *Pointer = nullptr;
};

/// Holder class for library handle
class SharedLibrary : public std::enable_shared_from_this<SharedLibrary> {
  SharedLibrary(const SharedLibrary &) = delete;
  SharedLibrary &operator=(const SharedLibrary &) = delete;
  SharedLibrary(SharedLibrary &&) = delete;
  SharedLibrary &operator=(SharedLibrary &&) = delete;

public:
#if WASMEDGE_OS_WINDOWS
  using NativeHandle = boost::winapi::HMODULE_;
#else
  using NativeHandle = void *;
#endif

  SharedLibrary() noexcept = default;
  ~SharedLibrary() noexcept { unload(); }
  Expect<void> load(const std::filesystem::path &Path) noexcept;
  void unload() noexcept;

  template <typename T> Symbol<T> get(const char *Name) {
    return Symbol<T>(shared_from_this(),
                     reinterpret_cast<T *>(getSymbolAddr(Name)));
  }

private:
  void *getSymbolAddr(const char *Name) const noexcept;
  NativeHandle Handle{};
};

} // namespace Loader
} // namespace WasmEdge
