// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p2/cli.h - wasi:cli host -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host component instances of `wasi:cli@0.2.12`:
/// `environment`, `exit`, the standard streams over `wasi:io`, and the
/// terminal interfaces. The functions shared with 0.3 come from its header.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "host/wasi/component/env.h"
#include "host/wasi/p2/io.h"
#include "runtime/component/callingframe.h"
#include "runtime/component/hostfunc.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/resource.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiP2 {

/// `environment.initial-cwd: func() -> option<string>`
class InitialCwd : public Runtime::Component::HostFunction<InitialCwd> {
public:
  Expect<std::optional<std::string>> body(Runtime::Component::CallingFrame &);
};

/// `stdin.get-stdin: func() -> input-stream`
class GetStdin : public Runtime::Component::HostFunction<GetStdin> {
public:
  GetStdin(IoHost &H) noexcept : Host(H) {}
  Expect<Runtime::Component::Own<InputStream>>
  body(Runtime::Component::CallingFrame &);

private:
  IoHost &Host;
};

/// `stdout.get-stdout: func() -> output-stream` and `stderr.get-stderr`
class GetStdout : public Runtime::Component::HostFunction<GetStdout> {
public:
  GetStdout(IoHost &H, uint32_t Fd) noexcept : Host(H), Fd(Fd) {}
  Expect<Runtime::Component::Own<OutputStream>>
  body(Runtime::Component::CallingFrame &);

private:
  IoHost &Host;
  uint32_t Fd;
};

/// `wasi:cli/environment@0.2.12`
class EnvironmentInstance : public Runtime::Instance::ComponentInstance {
public:
  EnvironmentInstance(WasiComponent::Env &Env);
};

/// `wasi:cli/exit@0.2.12`
class ExitInstance : public Runtime::Instance::ComponentInstance {
public:
  ExitInstance(WasiComponent::Env &Env);
};

/// `wasi:cli/stdin@0.2.12`
class StdinInstance : public Runtime::Instance::ComponentInstance {
public:
  StdinInstance(IoHost &Host);
};

/// `wasi:cli/stdout@0.2.12`
class StdoutInstance : public Runtime::Instance::ComponentInstance {
public:
  StdoutInstance(IoHost &Host);
};

/// `wasi:cli/stderr@0.2.12`
class StderrInstance : public Runtime::Instance::ComponentInstance {
public:
  StderrInstance(IoHost &Host);
};

/// `wasi:cli/terminal-input@0.2.12`: the resource type only.
class TerminalInputInstance : public Runtime::Instance::ComponentInstance {
public:
  TerminalInputInstance();
};

/// `wasi:cli/terminal-output@0.2.12`: the resource type only.
class TerminalOutputInstance : public Runtime::Instance::ComponentInstance {
public:
  TerminalOutputInstance();
};

/// `wasi:cli/terminal-stdin@0.2.12`
class TerminalStdinInstance : public Runtime::Instance::ComponentInstance {
public:
  TerminalStdinInstance(
      WasiComponent::Env &Env,
      const Runtime::Instance::Component::ResourceTypeInstance *InputType);
};

/// `wasi:cli/terminal-stdout@0.2.12`
class TerminalStdoutInstance : public Runtime::Instance::ComponentInstance {
public:
  TerminalStdoutInstance(
      WasiComponent::Env &Env,
      const Runtime::Instance::Component::ResourceTypeInstance *OutputType);
};

/// `wasi:cli/terminal-stderr@0.2.12`
class TerminalStderrInstance : public Runtime::Instance::ComponentInstance {
public:
  TerminalStderrInstance(
      WasiComponent::Env &Env,
      const Runtime::Instance::Component::ResourceTypeInstance *OutputType);
};

/// The ten instances of `wasi:cli@0.2.12`, the resource types first.
std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeCliInstances(WasiComponent::Env &Env, IoHost &Host);

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
