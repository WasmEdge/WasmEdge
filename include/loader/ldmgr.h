// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/loader/ldmgr.h - Loadable Manager definition -----------------===//
//
// Part of the SSVM Project.
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
#include "common/value.h"
#include "symbol.h"

#include <string_view>
#include <vector>

namespace SSVM {

/// Loadable manager interface.
class LDMgr {
public:
  /// Set the file path.
  Expect<void> setPath(const std::filesystem::path &FilePath);

  /// Read embedded Wasm binary.
  Expect<std::vector<Byte>> getWasm();

  /// Read ssvm version.
  Expect<uint32_t> getVersion();

  /// Get symbol.
  template <typename T = void> auto getSymbol(const char *Name) noexcept {
    return Handle->getSymbol<T>(Name);
  }

private:
  std::shared_ptr<DLHandle> Handle;
};

} // namespace SSVM
