// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/aot/cache.h - Cache class definition ---------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of Cache class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/span.h"
#include "common/types.h"

#include <string_view>

namespace WasmEdge {
namespace AOT {

/// Caching compiled module.
class Cache {
public:
  enum class StorageScope {
    Global,
    Local,
  };
  static Expect<std::filesystem::path>
  getPath(Span<const Byte> Data, StorageScope Scope, std::string_view Key = {});
  static void clear(StorageScope Scope, std::string_view Key = {});
};

} // namespace AOT
} // namespace WasmEdge
