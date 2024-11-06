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
  Expect<List<Tuple<std::string, std::string>>> body();
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

class Exit : public WasiCli<Exit> {
public:
  Exit(WasiCliEnvironment &HostEnv) : WasiCli(HostEnv) {}
  // TODO
  // exit: func(status: result);
  Expect<void> body();
};
class ExitWithCode : public WasiCli<ExitWithCode> {
public:
  ExitWithCode(WasiCliEnvironment &HostEnv) : WasiCli(HostEnv) {}
  // exit-with-code: func(status-code: u8);
  Expect<void> body(uint8_t StatusCode);
};

class GetStdin : public WasiCli<GetStdin> {
public:
  GetStdin(WasiCliEnvironment &HostEnv) : WasiCli(HostEnv) {}
  // TODO
  // 1. use wasi:io/streams@0.2.2.{input-stream};
  // 2. get-stdin: func() -> input-stream;
  Expect<void> body() { return {}; }
  // The problem here is, how to use a type that belongs to another plugin?
};

class GetStdout : public WasiCli<GetStdout> {
public:
  GetStdout(WasiCliEnvironment &HostEnv) : WasiCli(HostEnv) {}
  // TODO
  // get-stdout: func() -> output-stream;
  Expect<void> body() { return {}; }
};

class GetStderr : public WasiCli<GetStderr> {
public:
  GetStderr(WasiCliEnvironment &HostEnv) : WasiCli(HostEnv) {}
  // TODO
  // get-stderr: func() -> output-stream;
  Expect<void> body() { return {}; }
};

} // namespace Host
} // namespace WasmEdge
