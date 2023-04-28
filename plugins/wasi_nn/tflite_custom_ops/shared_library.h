// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

///
/// \file This file contains the declaration of the SharedLibrary, which holds
/// handle to loaded library. The function visibility is hidden in
/// libwasmedge.so, so cannot use the SharedLibrary in WasmEdge project.
///
#pragma once

#include "common/defines.h"
#include "common/log.h"

namespace WasmEdge {
namespace Host {
namespace WASINNTfLite {

#if WASMEDGE_OS_WINDOWS
#include <boost/winapi/dll.hpp>
using NativeHandle = boost::winapi::HMODULE_;
#else
using NativeHandle = void *;
#endif

class SharedLibrary {
public:
  SharedLibrary(const SharedLibrary &) = delete;

  SharedLibrary() noexcept = default;

  SharedLibrary(const char *Name) noexcept;

  static SharedLibrary loadFromEnv(const char *EnvKeyName) noexcept {
    if (auto *LibPath = ::getenv(EnvKeyName)) {
      return SharedLibrary(LibPath);
    }
    return SharedLibrary();
  }

  ~SharedLibrary() noexcept;

  void *getSymbolAddr(const char *Name) noexcept;

  bool isValid() noexcept { return Handle != nullptr; }

private:
  NativeHandle Handle{};
};

} // namespace WASINNTfLite
} // namespace Host
} // namespace WasmEdge
