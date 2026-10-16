// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "wast.h"

#include <filesystem>

namespace WasmEdge {
namespace Wast {

// Parse the JSON file of wast2json. The result holds no Source, because the
// commands refer to the module files next to the JSON file.
Expect<WastScript> parseJson(const std::filesystem::path &JsonPath);

} // namespace Wast
} // namespace WasmEdge
