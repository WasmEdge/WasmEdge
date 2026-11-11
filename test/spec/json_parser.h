// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "wast.h"

#include <filesystem>

namespace WasmEdge {
namespace Wast {

// Parse the JSON file of wast2json. The result holds no Source, because the
// commands refer to the module files next to the JSON file. The values of a
// component-model script are not core values. For such a script, set
// CoreValues to false: the commands then keep no argument and no expected
// value.
Expect<WastScript> parseJson(const std::filesystem::path &JsonPath,
                             bool CoreValues = true);

} // namespace Wast
} // namespace WasmEdge
