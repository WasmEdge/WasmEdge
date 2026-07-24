// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/types.h"

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Wast {

enum class CommandType {
  Module,
  ModuleDefinition,
  ModuleInstance,
  Register,
  Action,
  AssertReturn,
  AssertTrap,
  AssertExhaustion,
  AssertInvalid,
  AssertMalformed,
  AssertUnlinkable,
  AssertUninstantiable,
  AssertException,
  Thread,
  Wait,
};

enum class ActionType { Invoke, Get };

struct Action {
  ActionType Type;
  std::optional<std::string_view> ModuleName;
  std::string FieldName;
  std::vector<ValVariant> Args;
  std::vector<ValType> ArgTypes;
};

struct Result {
  ValVariant Value;
  ValType Type;

  enum class NaNPattern { None, Canonical, Arithmetic };
  NaNPattern NaN = NaNPattern::None;

  // Opaque reference check (ref.func, ref.extern, ...): only the type matters.
  bool OpaqueRef = false;

  // Bare (ref.null) without heap type: matches any null reference.
  bool AnyNullRef = false;

  // V128 lane shape, e.g. "f32x4"; empty if not V128.
  std::string V128Shape;
  std::vector<NaNPattern> V128LaneNaN; // per-lane NaN patterns; empty if none
};

struct ResultOrEither {
  std::vector<Result> Alternatives; // size 1 = normal, size 2+ = either
};

enum class ModuleType {
  TextFile,   // ModuleSource is a file path (JSON path)
  BinaryFile, // ModuleSource is a file path to .wasm (JSON path)
  Text,       // ModuleSource is ready WAT source
  Binary,     // ModuleSource is ready binary bytes
  Quote,      // ModuleSource is raw (module quote "...") text; resolved to Text
              // during parsing
};

struct ScriptCommand {
  CommandType Type;
  uint32_t Line = 0; // 1-based

  std::optional<std::string_view> ModuleName;
  std::optional<std::string_view> DefinitionName; // for ModuleInstance: $M

  std::string_view RegisterName;

  std::optional<Action> Act;

  std::vector<ResultOrEither> Expected;

  std::string_view ExpectedMessage;

  ModuleType ModType = ModuleType::Text;
  std::string_view ModuleSource;

  // For Thread commands: sub-commands to execute in the thread.
  std::vector<ScriptCommand> SubCommands;
  // For Thread commands: shared modules as (module_name, alias_name) pairs.
  std::vector<std::pair<std::string, std::string>> SharedModules;
  // For Wait commands: thread identifier to wait on.
  std::optional<std::string_view> ThreadName;
};

struct WastScript {
  // Owns the WAST source. ScriptCommands (and the converter) hold string_views
  // into it, so it lives behind a unique_ptr: the buffer address stays stable
  // across moves of the WastScript. A plain std::string member would not — a
  // move of a short (SSO) string relocates its bytes and dangles every view.
  std::unique_ptr<std::string> Source;
  std::vector<ScriptCommand> Commands;
  std::deque<std::string> OwnedStrings;

  WastScript() = default;
  WastScript(WastScript &&) noexcept = default;
  WastScript &operator=(WastScript &&) noexcept = default;
  // Copying would leave the copy's Commands/OwnedStrings string_views
  // pointing at the original's Source/OwnedStrings buffers.
  WastScript(const WastScript &) = delete;
  WastScript &operator=(const WastScript &) = delete;
};

} // namespace Wast
} // namespace WasmEdge
