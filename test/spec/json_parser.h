// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "wast.h"

#include <deque>
#include <filesystem>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Wast {

struct JsonScript {
  std::vector<ScriptCommand> Commands;
  std::deque<std::string> OwnedStrings;
};

Expect<JsonScript> parseJson(const std::filesystem::path &JsonPath);

} // namespace Wast
} // namespace WasmEdge
