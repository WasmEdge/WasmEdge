// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p3/cli.h - wasi:cli host -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host component instances of `wasi:cli@0.3.1`:
/// `environment`, `exit`, `types`, the standard streams, and the terminal
/// interfaces.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_valtype.h"
#include "common/errcode.h"
#include "common/expected.h"
#include "host/wasi/component/env.h"
#include "runtime/component/callingframe.h"
#include "runtime/component/hostfunc.h"
#include "runtime/component/wit.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/resource.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiP3 {

/// `wasi:cli/types@0.3.1` `error-code`.
enum class CliErrorCode : uint32_t { Io, IllegalByteSequence, Pipe };

/// The `terminal-input` and `terminal-output` resources carry no state.
struct TerminalInput {};
struct TerminalOutput {};

/// `result<_, error-code>` of the standard streams.
using StdioResult = Expected<Runtime::Component::Unit, CliErrorCode>;

} // namespace WasiP3
} // namespace Host

namespace Runtime {
namespace Component {

template <>
struct Wit<Host::WasiP3::CliErrorCode>
    : WitEnum<Host::WasiP3::CliErrorCode, Wit<Host::WasiP3::CliErrorCode>> {
  static constexpr const char *Labels[] = {"io", "illegal-byte-sequence",
                                           "pipe"};
};

} // namespace Component
} // namespace Runtime

namespace Host {
namespace WasiP3 {

/// `environment.get-environment: func() -> list<tuple<string, string>>`
class GetEnvironment : public Runtime::Component::HostFunction<GetEnvironment> {
public:
  GetEnvironment(WasiComponent::Env &E) noexcept : Env(E) {}
  Expect<std::vector<std::tuple<std::string, std::string>>>
  body(Runtime::Component::CallingFrame &);

private:
  WasiComponent::Env &Env;
};

/// `environment.get-arguments: func() -> list<string>`
class GetArguments : public Runtime::Component::HostFunction<GetArguments> {
public:
  GetArguments(WasiComponent::Env &E) noexcept : Env(E) {}
  Expect<std::vector<std::string>> body(Runtime::Component::CallingFrame &);

private:
  WasiComponent::Env &Env;
};

/// `environment.get-initial-cwd: func() -> option<string>`
class GetInitialCwd : public Runtime::Component::HostFunction<GetInitialCwd> {
public:
  Expect<std::optional<std::string>> body(Runtime::Component::CallingFrame &);
};

/// `exit.exit: func(status: result)`
class Exit : public Runtime::Component::HostFunction<Exit> {
public:
  static constexpr const char *ParamNames[] = {"status"};
  Exit(WasiComponent::Env &E) noexcept : Env(E) {}
  Expect<void>
  body(Runtime::Component::CallingFrame &,
       Expected<Runtime::Component::Unit, Runtime::Component::Unit> Status);

private:
  WasiComponent::Env &Env;
};

/// `exit.exit-with-code: func(status-code: u8)`
class ExitWithCode : public Runtime::Component::HostFunction<ExitWithCode> {
public:
  static constexpr const char *ParamNames[] = {"status-code"};
  ExitWithCode(WasiComponent::Env &E) noexcept : Env(E) {}
  Expect<void> body(Runtime::Component::CallingFrame &, uint8_t StatusCode);

private:
  WasiComponent::Env &Env;
};

/// `stdin.read-via-stream: func() -> tuple<stream<u8>, future<result<_,
/// error-code>>>`: a host task feeds the stream from the standard input.
class ReadViaStream : public Runtime::Component::HostFunction<ReadViaStream> {
public:
  ReadViaStream(WasiComponent::Env &E, uint32_t Fd) noexcept : Env(E), Fd(Fd) {}
  void declare(const Runtime::Component::TypeMinter &Mint) noexcept override;
  Expect<std::tuple<Runtime::Component::Stream<uint8_t>,
                    Runtime::Component::Future<StdioResult>>>
  body(Runtime::Component::CallingFrame &Frame);

private:
  WasiComponent::Env &Env;
  uint32_t Fd;
  ComponentValType ResultType;
};

/// `stdout.write-via-stream: func(data: stream<u8>) -> future<result<_,
/// error-code>>` (and the same for `stderr`): the stream sinks into the
/// standard output, and the future resolves once the guest drops it.
class WriteViaStream : public Runtime::Component::HostFunction<WriteViaStream> {
public:
  static constexpr const char *ParamNames[] = {"data"};
  WriteViaStream(WasiComponent::Env &E, uint32_t Fd) noexcept
      : Env(E), Fd(Fd) {}
  void declare(const Runtime::Component::TypeMinter &Mint) noexcept override;
  Expect<Runtime::Component::Future<StdioResult>>
  body(Runtime::Component::CallingFrame &Frame,
       Runtime::Component::Stream<uint8_t> Data);

private:
  WasiComponent::Env &Env;
  uint32_t Fd;
  ComponentValType ResultType;
};

/// `terminal-stdin.get-terminal-stdin: func() -> option<terminal-input>`
class GetTerminalInput
    : public Runtime::Component::HostFunction<GetTerminalInput> {
public:
  GetTerminalInput(WasiComponent::Env &E, uint32_t Fd) noexcept
      : Env(E), Fd(Fd) {}
  Expect<std::optional<Runtime::Component::Own<TerminalInput>>>
  body(Runtime::Component::CallingFrame &);

private:
  WasiComponent::Env &Env;
  uint32_t Fd;
};

/// `terminal-stdout.get-terminal-stdout: func() -> option<terminal-output>`
/// (and the same for `terminal-stderr`)
class GetTerminalOutput
    : public Runtime::Component::HostFunction<GetTerminalOutput> {
public:
  GetTerminalOutput(WasiComponent::Env &E, uint32_t Fd) noexcept
      : Env(E), Fd(Fd) {}
  Expect<std::optional<Runtime::Component::Own<TerminalOutput>>>
  body(Runtime::Component::CallingFrame &);

private:
  WasiComponent::Env &Env;
  uint32_t Fd;
};

/// `wasi:cli/environment@0.3.1`
class EnvironmentInstance : public Runtime::Instance::ComponentInstance {
public:
  EnvironmentInstance(WasiComponent::Env &Env);
};

/// `wasi:cli/exit@0.3.1`
class ExitInstance : public Runtime::Instance::ComponentInstance {
public:
  ExitInstance(WasiComponent::Env &Env);
};

/// `wasi:cli/types@0.3.1`: type-only.
class CliTypesInstance : public Runtime::Instance::ComponentInstance {
public:
  CliTypesInstance();
};

/// `wasi:cli/stdin@0.3.1`
class StdinInstance : public Runtime::Instance::ComponentInstance {
public:
  StdinInstance(WasiComponent::Env &Env);
};

/// `wasi:cli/stdout@0.3.1`
class StdoutInstance : public Runtime::Instance::ComponentInstance {
public:
  StdoutInstance(WasiComponent::Env &Env);
};

/// `wasi:cli/stderr@0.3.1`
class StderrInstance : public Runtime::Instance::ComponentInstance {
public:
  StderrInstance(WasiComponent::Env &Env);
};

/// `wasi:cli/terminal-input@0.3.1`: the resource type only.
class TerminalInputInstance : public Runtime::Instance::ComponentInstance {
public:
  TerminalInputInstance();
};

/// `wasi:cli/terminal-output@0.3.1`: the resource type only.
class TerminalOutputInstance : public Runtime::Instance::ComponentInstance {
public:
  TerminalOutputInstance();
};

/// `wasi:cli/terminal-stdin@0.3.1`, over the resource type of
/// `terminal-input`.
class TerminalStdinInstance : public Runtime::Instance::ComponentInstance {
public:
  TerminalStdinInstance(
      WasiComponent::Env &Env,
      const Runtime::Instance::Component::ResourceTypeInstance *InputType);
};

/// `wasi:cli/terminal-stdout@0.3.1`, over the resource type of
/// `terminal-output`.
class TerminalStdoutInstance : public Runtime::Instance::ComponentInstance {
public:
  TerminalStdoutInstance(
      WasiComponent::Env &Env,
      const Runtime::Instance::Component::ResourceTypeInstance *OutputType);
};

/// `wasi:cli/terminal-stderr@0.3.1`, over the resource type of
/// `terminal-output`.
class TerminalStderrInstance : public Runtime::Instance::ComponentInstance {
public:
  TerminalStderrInstance(
      WasiComponent::Env &Env,
      const Runtime::Instance::Component::ResourceTypeInstance *OutputType);
};

/// The eleven instances of `wasi:cli@0.3.1`, the resource types first.
std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeCliInstances(WasiComponent::Env &Env);

} // namespace WasiP3
} // namespace Host
} // namespace WasmEdge
