// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
#include "common/executable.h"
#include "common/filesystem.h"
#include "system/winapi.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace WasmEdge {
namespace Loader {

/// Holder class for library handle
class SharedLibrary : public Executable {
public:
#if WASMEDGE_OS_WINDOWS
  using NativeHandle = winapi::HMODULE_;
#else
  using NativeHandle = void *;
#endif

  SharedLibrary() noexcept = default;
  ~SharedLibrary() noexcept override { unload(); }
  Expect<void> load(const std::filesystem::path &Path) noexcept;
  void unload() noexcept;

  Symbol<const IntrinsicsTable *> getIntrinsics() noexcept override {
    return get<const IntrinsicsTable *>("intrinsics");
  }

  std::vector<Symbol<Wrapper>> getTypes(size_t Size) noexcept override {
    using namespace std::literals;
    std::vector<Symbol<Wrapper>> Result;
    Result.reserve(Size);
    for (size_t I = 0; I < Size; ++I) {
      // "t" prefix is for type helper function
      const std::string Name = fmt::format("t{}"sv, I);
      if (auto Symbol = get<Wrapper>(Name.c_str())) {
        Result.push_back(std::move(Symbol));
      }
    }

    return Result;
  }

  std::vector<Symbol<void>> getCodes(size_t Offset,
                                     size_t Size) noexcept override {
    using namespace std::literals;
    std::vector<Symbol<void>> Result;
    Result.reserve(Size);
    for (size_t I = 0; I < Size; ++I) {
      // "f" prefix is for code function
      const std::string Name = fmt::format("f{}"sv, I + Offset);
      if (auto Symbol = get<void>(Name.c_str())) {
        Result.push_back(std::move(Symbol));
      }
    }

    return Result;
  }

  /// Read embedded Wasm binary.
  Expect<std::vector<Byte>> getWasm() noexcept {
    const auto Size = get<uint32_t>("wasm.size");
    if (unlikely(!Size)) {
      spdlog::error(ErrCode::Value::IllegalGrammar);
      return Unexpect(ErrCode::Value::IllegalGrammar);
    }
    const auto Code = get<uint8_t>("wasm.code");
    if (unlikely(!Code)) {
      spdlog::error(ErrCode::Value::IllegalGrammar);
      return Unexpect(ErrCode::Value::IllegalGrammar);
    }

    return std::vector<Byte>(Code.get(), Code.get() + *Size);
  }

  /// Read wasmedge version.
  Expect<uint32_t> getVersion() noexcept {
    const auto Version = get<uint32_t>("version");
    if (unlikely(!Version)) {
      spdlog::error(ErrCode::Value::IllegalGrammar);
      return Unexpect(ErrCode::Value::IllegalGrammar);
    }
    return *Version;
  }

  template <typename T> Symbol<T> get(const char *Name) {
    return createSymbol<T>(reinterpret_cast<T *>(getSymbolAddr(Name)));
  }

private:
  void *getSymbolAddr(const char *Name) const noexcept;
  NativeHandle Handle{};
};

} // namespace Loader
} // namespace WasmEdge
