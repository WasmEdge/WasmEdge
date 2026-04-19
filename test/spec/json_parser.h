// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#pragma once

#include "common/errcode.h"
#include "wast.h"

#include "simdjson.h"

#include <deque>
#include <filesystem>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Wast {

struct JsonScript {
  std::vector<ScriptCommand> Commands;
  std::deque<std::string> OwnedStrings;
  simdjson::dom::parser Parser;
  simdjson::dom::element Doc;
};

Expect<JsonScript> parseJson(const std::filesystem::path &JsonPath);

} // namespace Wast
} // namespace WasmEdge
