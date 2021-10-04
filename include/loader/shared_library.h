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

namespace WasmEdge::AST {
class AOTSection;
}

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

  explicit Symbol(T *S) noexcept : Pointer(S) {}

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
  Expect<void> load(const AST::AOTSection &AOTSec) noexcept;
  void unload() noexcept;

  template <typename T> Symbol<T> get(const char *Name) {
    return Symbol<T>(shared_from_this(),
                     reinterpret_cast<T *>(getSymbolAddr(Name)));
  }

  uintptr_t getOffset() const noexcept;

  template <typename T> T *getPointer(uint64_t Address) const noexcept {
    if (unlikely(Address == 0)) {
      return nullptr;
    }
    return reinterpret_cast<T *>(getOffset() + Address);
  }

  template <typename T> Symbol<T> getIntrinsics() noexcept {
    if (Binary && IntrinsicsAddress != 0) {
      return Symbol<T>(shared_from_this(), getPointer<T>(IntrinsicsAddress));
    }
    return {};
  }

  template <typename T> std::vector<Symbol<T>> getTypes() noexcept {
    std::vector<Symbol<T>> Result;
    if (Binary) {
      Result.reserve(TypesAddress.size());
      for (const auto Address : TypesAddress) {
        Result.push_back(Symbol<T>(shared_from_this(), getPointer<T>(Address)));
      }
    }
    return Result;
  }

  template <typename T> std::vector<Symbol<T>> getCodes() noexcept {
    std::vector<Symbol<T>> Result;
    if (Binary) {
      Result.reserve(CodesAddress.size());
      for (const auto Address : CodesAddress) {
        Result.push_back(Symbol<T>(shared_from_this(), getPointer<T>(Address)));
      }
    }
    return Result;
  }

private:
  void *getSymbolAddr(const char *Name) const noexcept;
  NativeHandle Handle{};

  uint8_t *Binary = nullptr;
  uint64_t BinarySize = 0;
  uint64_t IntrinsicsAddress = 0;
  std::vector<uintptr_t> TypesAddress;
  std::vector<uintptr_t> CodesAddress;
};

} // namespace Loader
} // namespace WasmEdge
