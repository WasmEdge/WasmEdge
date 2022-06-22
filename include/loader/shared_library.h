// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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

#include "ast/section.h"
#include "common/defines.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/symbol.h"

#include <cstdint>
#include <memory>
#include <vector>

#if WASMEDGE_OS_WINDOWS
#include <boost/winapi/dll.hpp>
#endif

namespace WasmEdge {
namespace Loader {

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
    return reinterpret_cast<T *>(getOffset() + Address);
  }

  template <typename T> Symbol<T> getIntrinsics() noexcept {
    if (Binary) {
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
