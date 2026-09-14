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

  // This is an opaque reference, such as ref.func or ref.extern. Only the type
  // is important.
  bool OpaqueRef = false;

  // This is a bare (ref.null) with no heap type. It matches every null
  // reference.
  bool AnyNullRef = false;

  // This is the V128 lane shape, for example "f32x4". It is empty for a value
  // that is not a V128.
  std::string V128Shape;
  // This holds the NaN pattern of each lane. It is empty if there is no such
  // pattern.
  std::vector<NaNPattern> V128LaneNaN;
};

struct ResultOrEither {
  // A size of 1 gives a normal result. A size of 2 or more gives an either
  // result.
  std::vector<Result> Alternatives;
};

enum class ModuleType {
  TextFile,   // ModuleSource is a file path from the JSON.
  BinaryFile, // ModuleSource is a file path to a .wasm file from the JSON.
  Text,       // ModuleSource is WAT source that is ready.
  Binary,     // ModuleSource is binary bytes that are ready.
  Quote,      // ModuleSource is raw (module quote "...") text. The parser
              // changes it into Text.
};

struct ScriptCommand {
  CommandType Type;
  uint32_t Line = 0; // The first line is line 1.

  std::optional<std::string_view> ModuleName;
  // This is the definition name of a ModuleInstance, such as $M.
  std::optional<std::string_view> DefinitionName;

  std::string_view RegisterName;

  std::optional<Action> Act;

  std::vector<ResultOrEither> Expected;

  std::string_view ExpectedMessage;

  ModuleType ModType = ModuleType::Text;
  std::string_view ModuleSource;

  // A Thread command runs these sub-commands in the thread.
  std::vector<ScriptCommand> SubCommands;
  // A Thread command shares these modules. Each pair holds a module name and
  // an alias name.
  std::vector<std::pair<std::string, std::string>> SharedModules;
  // A Wait command waits for the thread with this identifier.
  std::optional<std::string_view> ThreadName;
};

struct WastScript {
  // This member owns the WAST source. The ScriptCommands and the converter
  // hold string_views into it, so a unique_ptr holds it. The address of the
  // buffer then stays stable when the code moves the WastScript. A plain
  // std::string member does not give this property. A move of a short SSO
  // string moves its bytes, and every view then dangles.
  std::unique_ptr<std::string> Source;
  std::vector<ScriptCommand> Commands;
  std::deque<std::string> OwnedStrings;

  WastScript() = default;
  WastScript(WastScript &&) noexcept = default;
  WastScript &operator=(WastScript &&) noexcept = default;
  // A copy is not permitted. The Commands and OwnedStrings string_views of the
  // copy point into the Source and OwnedStrings buffers of the original.
  WastScript(const WastScript &) = delete;
  WastScript &operator=(const WastScript &) = delete;
};

} // namespace Wast
} // namespace WasmEdge
