// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "ast/module.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "wast.h"

namespace WasmEdge {
namespace Wast {

Expect<WastScript> parseWast(const std::filesystem::path &Path);

} // namespace Wast
} // namespace WasmEdge
