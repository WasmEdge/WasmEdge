// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/component_valtype.h"
#include "host/wasi/p3/cli.h"
#include "runtime/component/hosttransmit.h"

#include <array>
#include <memory>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiP3 {

namespace {

CliErrorCode errorCodeOf(__wasi_errno_t Errno) noexcept {
  switch (Errno) {
  case __WASI_ERRNO_ILSEQ:
    return CliErrorCode::IllegalByteSequence;
  case __WASI_ERRNO_PIPE:
    return CliErrorCode::Pipe;
  default:
    return CliErrorCode::Io;
  }
}

// Write all of Data to Node.
WASI::WasiExpect<void> writeAll(WASI::VINode &Node,
                                Span<const uint8_t> Data) noexcept {
  while (!Data.empty()) {
    std::array<Span<const uint8_t>, 1> IOVs{Data};
    __wasi_size_t Written = 0;
    EXPECTED_TRY(Node.fdWrite(IOVs, Written));
    if (Written == 0) {
      return WASI::WasiUnexpect(__WASI_ERRNO_IO);
    }
    Data = Data.subspan(Written);
  }
  return {};
}

} // namespace

Expect<std::vector<std::tuple<std::string, std::string>>>
GetEnvironment::body(Runtime::Component::CallingFrame &) {
  std::vector<std::tuple<std::string, std::string>> Out;
  for (const auto &Entry : Env.preview1().getEnvironVariables()) {
    const auto Eq = Entry.find('=');
    if (Eq == std::string::npos) {
      Out.emplace_back(Entry, std::string());
    } else {
      Out.emplace_back(Entry.substr(0, Eq), Entry.substr(Eq + 1));
    }
  }
  return Out;
}

Expect<std::vector<std::string>>
GetArguments::body(Runtime::Component::CallingFrame &) {
  return Env.preview1().getArguments();
}

Expect<std::optional<std::string>>
GetInitialCwd::body(Runtime::Component::CallingFrame &) {
  return std::optional<std::string>();
}

Expect<void> Exit::body(
    Runtime::Component::CallingFrame &,
    Expected<Runtime::Component::Unit, Runtime::Component::Unit> Status) {
  Env.preview1().procExit(Status.has_value() ? 0 : 1);
  return Unexpect(ErrCode::Value::Terminated);
}

Expect<void> ExitWithCode::body(Runtime::Component::CallingFrame &,
                                uint8_t StatusCode) {
  Env.preview1().procExit(StatusCode);
  return Unexpect(ErrCode::Value::Terminated);
}

void ReadViaStream::declare(
    const Runtime::Component::TypeMinter &Mint) noexcept {
  HostFunction::declare(Mint);
  ResultType = Runtime::Component::Wit<StdioResult>::type(Mint);
}

Expect<std::tuple<Runtime::Component::Stream<uint8_t>,
                  Runtime::Component::Future<StdioResult>>>
ReadViaStream::body(Runtime::Component::CallingFrame &Frame) {
  auto Node = Env.stdioNode(Fd);
  auto Data = Runtime::Component::HostTransmitEnd::newWritable(
      Frame, true, ComponentValType(ComponentTypeCode::U8));
  auto Done = Runtime::Component::HostTransmitEnd::newWritable(Frame, false,
                                                               ResultType);
  std::tuple<Runtime::Component::Stream<uint8_t>,
             Runtime::Component::Future<StdioResult>>
      Ends{Runtime::Component::Stream<uint8_t>{Data->getGuestValue()},
           Runtime::Component::Future<StdioResult>{Done->getGuestValue()}};
  if (!Node) {
    Data->drop();
    Done->writeDetached(Runtime::Component::Wit<StdioResult>::into(
        Unexpected<CliErrorCode>(CliErrorCode::Io)));
    return Ends;
  }
  Frame.spawn([Node, Data,
               Done](Runtime::Component::CallingFrame &F) -> Expect<void> {
    auto Buffer = std::make_shared<std::vector<uint8_t>>(65536);
    auto Read = std::make_shared<WASI::WasiExpect<__wasi_size_t>>(0);
    StdioResult Result{Runtime::Component::Unit{}};
    while (true) {
      EXPECTED_TRY(F.blocking([Node, Buffer, Read]() {
        std::array<Span<uint8_t>, 1> IOVs{Span<uint8_t>(*Buffer)};
        __wasi_size_t Count = 0;
        if (auto Res = Node->fdRead(IOVs, Count); !Res) {
          *Read = WASI::WasiUnexpect(Res.error());
        } else {
          *Read = Count;
        }
      }));
      if (!*Read) {
        Result = Unexpected<CliErrorCode>(errorCodeOf(Read->error()));
        break;
      }
      if (**Read == 0) {
        break;
      }
      EXPECTED_TRY(auto Outcome,
                   Data->write(F, Span<const uint8_t>(Buffer->data(), **Read)));
      if (Outcome.Result !=
          Runtime::Instance::Component::TransmitResult::Completed) {
        break;
      }
    }
    Data->drop();
    Done->writeDetached(
        Runtime::Component::Wit<StdioResult>::into(std::move(Result)));
    return {};
  });
  return Ends;
}

void WriteViaStream::declare(
    const Runtime::Component::TypeMinter &Mint) noexcept {
  HostFunction::declare(Mint);
  ResultType = Runtime::Component::Wit<StdioResult>::type(Mint);
}

Expect<Runtime::Component::Future<StdioResult>>
WriteViaStream::body(Runtime::Component::CallingFrame &Frame,
                     Runtime::Component::Stream<uint8_t> Data) {
  auto Node = Env.stdioNode(Fd);
  auto In =
      Runtime::Component::HostTransmitEnd::adoptReadable(Frame, Data.Shared);
  auto Done = Runtime::Component::HostTransmitEnd::newWritable(Frame, false,
                                                               ResultType);
  Runtime::Component::Future<StdioResult> Fut{Done->getGuestValue()};
  auto Failure = std::make_shared<CliErrorCode>(CliErrorCode::Io);
  In->sink(
      [Node, Failure](Span<const uint8_t> Bytes) {
        if (!Node) {
          return false;
        }
        if (auto Res = writeAll(*Node, Bytes); !Res) {
          *Failure = errorCodeOf(Res.error());
          return false;
        }
        return true;
      },
      [Done, Failure](bool Failed) {
        StdioResult Result =
            Failed ? StdioResult(Unexpected<CliErrorCode>(*Failure))
                   : StdioResult(Runtime::Component::Unit{});
        Done->writeDetached(
            Runtime::Component::Wit<StdioResult>::into(std::move(Result)));
      });
  return Fut;
}

Expect<std::optional<Runtime::Component::Own<TerminalInput>>>
GetTerminalInput::body(Runtime::Component::CallingFrame &) {
  if (!Env.isTerminal(Fd)) {
    return std::optional<Runtime::Component::Own<TerminalInput>>();
  }
  return std::optional<Runtime::Component::Own<TerminalInput>>(
      Runtime::Component::Own<TerminalInput>{1});
}

Expect<std::optional<Runtime::Component::Own<TerminalOutput>>>
GetTerminalOutput::body(Runtime::Component::CallingFrame &) {
  if (!Env.isTerminal(Fd)) {
    return std::optional<Runtime::Component::Own<TerminalOutput>>();
  }
  return std::optional<Runtime::Component::Own<TerminalOutput>>(
      Runtime::Component::Own<TerminalOutput>{1});
}

EnvironmentInstance::EnvironmentInstance(WasiComponent::Env &Env)
    : ComponentInstance("wasi:cli/environment@0.3.1") {
  addHostFunc("get-environment", std::make_unique<GetEnvironment>(Env));
  addHostFunc("get-arguments", std::make_unique<GetArguments>(Env));
  addHostFunc("get-initial-cwd", std::make_unique<GetInitialCwd>());
}

ExitInstance::ExitInstance(WasiComponent::Env &Env)
    : ComponentInstance("wasi:cli/exit@0.3.1") {
  addHostFunc("exit", std::make_unique<Exit>(Env));
  addHostFunc("exit-with-code", std::make_unique<ExitWithCode>(Env));
}

CliTypesInstance::CliTypesInstance()
    : ComponentInstance("wasi:cli/types@0.3.1") {
  exportType("error-code",
             Runtime::Component::Wit<CliErrorCode>::type(getTypeMinter())
                 .getTypeIndex());
}

StdinInstance::StdinInstance(WasiComponent::Env &Env)
    : ComponentInstance("wasi:cli/stdin@0.3.1") {
  exportType("error-code",
             Runtime::Component::Wit<CliErrorCode>::type(getTypeMinter())
                 .getTypeIndex());
  addHostFunc("read-via-stream", std::make_unique<ReadViaStream>(Env, 0));
}

StdoutInstance::StdoutInstance(WasiComponent::Env &Env)
    : ComponentInstance("wasi:cli/stdout@0.3.1") {
  exportType("error-code",
             Runtime::Component::Wit<CliErrorCode>::type(getTypeMinter())
                 .getTypeIndex());
  addHostFunc("write-via-stream", std::make_unique<WriteViaStream>(Env, 1));
}

StderrInstance::StderrInstance(WasiComponent::Env &Env)
    : ComponentInstance("wasi:cli/stderr@0.3.1") {
  exportType("error-code",
             Runtime::Component::Wit<CliErrorCode>::type(getTypeMinter())
                 .getTypeIndex());
  addHostFunc("write-via-stream", std::make_unique<WriteViaStream>(Env, 2));
}

TerminalInputInstance::TerminalInputInstance()
    : ComponentInstance("wasi:cli/terminal-input@0.3.1") {
  exportType("terminal-input",
             addHostResourceType<TerminalInput>([](uint64_t) {}));
}

TerminalOutputInstance::TerminalOutputInstance()
    : ComponentInstance("wasi:cli/terminal-output@0.3.1") {
  exportType("terminal-output",
             addHostResourceType<TerminalOutput>([](uint64_t) {}));
}

TerminalStdinInstance::TerminalStdinInstance(
    WasiComponent::Env &Env,
    const Runtime::Instance::Component::ResourceTypeInstance *InputType)
    : ComponentInstance("wasi:cli/terminal-stdin@0.3.1") {
  exportType("terminal-input", addSharedResourceType<TerminalInput>(InputType));
  addHostFunc("get-terminal-stdin", std::make_unique<GetTerminalInput>(Env, 0));
}

TerminalStdoutInstance::TerminalStdoutInstance(
    WasiComponent::Env &Env,
    const Runtime::Instance::Component::ResourceTypeInstance *OutputType)
    : ComponentInstance("wasi:cli/terminal-stdout@0.3.1") {
  exportType("terminal-output",
             addSharedResourceType<TerminalOutput>(OutputType));
  addHostFunc("get-terminal-stdout",
              std::make_unique<GetTerminalOutput>(Env, 1));
}

TerminalStderrInstance::TerminalStderrInstance(
    WasiComponent::Env &Env,
    const Runtime::Instance::Component::ResourceTypeInstance *OutputType)
    : ComponentInstance("wasi:cli/terminal-stderr@0.3.1") {
  exportType("terminal-output",
             addSharedResourceType<TerminalOutput>(OutputType));
  addHostFunc("get-terminal-stderr",
              std::make_unique<GetTerminalOutput>(Env, 2));
}

std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeCliInstances(WasiComponent::Env &Env) {
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Out;
  auto Input = std::make_unique<TerminalInputInstance>();
  auto Output = std::make_unique<TerminalOutputInstance>();
  const auto *InputType = Input->findTypeResource("terminal-input");
  const auto *OutputType = Output->findTypeResource("terminal-output");
  Out.push_back(std::move(Input));
  Out.push_back(std::move(Output));
  Out.push_back(std::make_unique<CliTypesInstance>());
  Out.push_back(std::make_unique<EnvironmentInstance>(Env));
  Out.push_back(std::make_unique<ExitInstance>(Env));
  Out.push_back(std::make_unique<StdinInstance>(Env));
  Out.push_back(std::make_unique<StdoutInstance>(Env));
  Out.push_back(std::make_unique<StderrInstance>(Env));
  Out.push_back(std::make_unique<TerminalStdinInstance>(Env, InputType));
  Out.push_back(std::make_unique<TerminalStdoutInstance>(Env, OutputType));
  Out.push_back(std::make_unique<TerminalStderrInstance>(Env, OutputType));
  return Out;
}

} // namespace WasiP3
} // namespace Host
} // namespace WasmEdge
