// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/loader/ldmgr.h - Loadable Manager definition -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the LDMgr class, which controls flow
/// of Compiled Binary loading.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/types.h"
#include "shared_library.h"

#include <string_view>
#include <vector>

namespace WasmEdge {

/// Loadable manager interface.
class LDMgr {
public:
  LDMgr(const void *IT = nullptr) : Intrinsics(IT) {}

  /// Set the file path.
  Expect<void> setPath(const std::filesystem::path &FilePath);

  /// Read embedded Wasm binary.
  Expect<std::vector<Byte>> getWasm();

  /// Read wasmedge version.
  Expect<uint32_t> getVersion();

  /// Get symbol.
  template <typename T = void> auto getSymbol(const char *Name) noexcept {
    return Library->get<T>(Name);
  }

private:
  std::shared_ptr<Loader::SharedLibrary> Library;
  const void *Intrinsics;
};

} // namespace WasmEdge
