// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {

class GetEnvironment : public WasiCli<GetEnvironment> {
public:
  GetEnvironment(WasiCliEnvironment &HostEnv) : WasiCli(HostEnv) {}
  // func() -> list<tuple<string, string>>
  // TODO: fix the type, need to introduce tuple type
  Expect<void> body() { return {}; }
};

class GetArguments : public WasiCli<GetArguments> {
public:
  GetArguments(WasiCliEnvironment &HostEnv) : WasiCli(HostEnv) {}
  // get-arguments: func() -> list<string>;
  Expect<List<std::string>> body();
};

class InitialCwd : public WasiCli<InitialCwd> {
public:
  InitialCwd(WasiCliEnvironment &HostEnv) : WasiCli(HostEnv) {}
  // initial-cwd: func() -> option<string>;
  Expect<std::string> body();
};

} // namespace Host
} // namespace WasmEdge
