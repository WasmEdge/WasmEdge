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
#include "common/types.h"
#include "common/value.h"

#include <fstream>
#include <string>
#include <vector>

namespace SSVM {

/// Loadable manager interface.
class LDMgr {
public:
  ~LDMgr() noexcept;

  /// Set the file path.
  Expect<void> setPath(std::string_view FilePath);

  /// Read embedded Wasm binary.
  Expect<std::vector<Byte>> getWasm();

  /// Read ssvm version.
  Expect<uint32_t> getVersion();

  /// Get symbol.
  template <typename T> T *getSymbol(const char *Name) {
    return reinterpret_cast<T *>(getRawSymbol(Name));
  }
  void *getRawSymbol(const char *Name);

private:
  void *Handler = nullptr;
};

} // namespace SSVM
