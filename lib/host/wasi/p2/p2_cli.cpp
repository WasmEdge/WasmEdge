// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "host/wasi/p2/cli.h"
#include "host/wasi/p3/cli.h"

#include <memory>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiP2 {

Expect<std::optional<std::string>>
InitialCwd::body(Runtime::Component::CallingFrame &) {
  return std::optional<std::string>();
}

Expect<Runtime::Component::Own<InputStream>>
GetStdin::body(Runtime::Component::CallingFrame &) {
  auto Node = Host.env().stdioNode(0);
  auto Stream = std::make_shared<NodeInputStream>(Host, std::move(Node),
                                                  NodeInputStream::Mode::Fd);
  return Runtime::Component::Own<InputStream>{
      Host.inputs().add(std::move(Stream))};
}

Expect<Runtime::Component::Own<OutputStream>>
GetStdout::body(Runtime::Component::CallingFrame &) {
  auto Node = Host.env().stdioNode(Fd);
  auto Stream = std::make_shared<NodeOutputStream>(Host, std::move(Node),
                                                   NodeOutputStream::Mode::Fd);
  return Runtime::Component::Own<OutputStream>{
      Host.outputs().add(std::move(Stream))};
}

EnvironmentInstance::EnvironmentInstance(WasiComponent::Env &Env)
    : ComponentInstance("wasi:cli/environment@0.2.12") {
  addHostFunc("get-environment", std::make_unique<WasiP3::GetEnvironment>(Env));
  addHostFunc("get-arguments", std::make_unique<WasiP3::GetArguments>(Env));
  addHostFunc("initial-cwd", std::make_unique<InitialCwd>());
}

ExitInstance::ExitInstance(WasiComponent::Env &Env)
    : ComponentInstance("wasi:cli/exit@0.2.12") {
  addHostFunc("exit", std::make_unique<WasiP3::Exit>(Env));
  addHostFunc("exit-with-code", std::make_unique<WasiP3::ExitWithCode>(Env));
}

StdinInstance::StdinInstance(IoHost &Host)
    : ComponentInstance("wasi:cli/stdin@0.2.12") {
  exportType("input-stream",
             addSharedResourceType<InputStream>(Host.InputType));
  addHostFunc("get-stdin", std::make_unique<GetStdin>(Host));
}

StdoutInstance::StdoutInstance(IoHost &Host)
    : ComponentInstance("wasi:cli/stdout@0.2.12") {
  exportType("output-stream",
             addSharedResourceType<OutputStream>(Host.OutputType));
  addHostFunc("get-stdout", std::make_unique<GetStdout>(Host, 1));
}

StderrInstance::StderrInstance(IoHost &Host)
    : ComponentInstance("wasi:cli/stderr@0.2.12") {
  exportType("output-stream",
             addSharedResourceType<OutputStream>(Host.OutputType));
  addHostFunc("get-stderr", std::make_unique<GetStdout>(Host, 2));
}

TerminalInputInstance::TerminalInputInstance()
    : ComponentInstance("wasi:cli/terminal-input@0.2.12") {
  exportType("terminal-input",
             addHostResourceType<WasiP3::TerminalInput>([](uint64_t) {}));
}

TerminalOutputInstance::TerminalOutputInstance()
    : ComponentInstance("wasi:cli/terminal-output@0.2.12") {
  exportType("terminal-output",
             addHostResourceType<WasiP3::TerminalOutput>([](uint64_t) {}));
}

TerminalStdinInstance::TerminalStdinInstance(
    WasiComponent::Env &Env,
    const Runtime::Instance::Component::ResourceTypeInstance *InputType)
    : ComponentInstance("wasi:cli/terminal-stdin@0.2.12") {
  exportType("terminal-input",
             addSharedResourceType<WasiP3::TerminalInput>(InputType));
  addHostFunc("get-terminal-stdin",
              std::make_unique<WasiP3::GetTerminalInput>(Env, 0));
}

TerminalStdoutInstance::TerminalStdoutInstance(
    WasiComponent::Env &Env,
    const Runtime::Instance::Component::ResourceTypeInstance *OutputType)
    : ComponentInstance("wasi:cli/terminal-stdout@0.2.12") {
  exportType("terminal-output",
             addSharedResourceType<WasiP3::TerminalOutput>(OutputType));
  addHostFunc("get-terminal-stdout",
              std::make_unique<WasiP3::GetTerminalOutput>(Env, 1));
}

TerminalStderrInstance::TerminalStderrInstance(
    WasiComponent::Env &Env,
    const Runtime::Instance::Component::ResourceTypeInstance *OutputType)
    : ComponentInstance("wasi:cli/terminal-stderr@0.2.12") {
  exportType("terminal-output",
             addSharedResourceType<WasiP3::TerminalOutput>(OutputType));
  addHostFunc("get-terminal-stderr",
              std::make_unique<WasiP3::GetTerminalOutput>(Env, 2));
}

std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeCliInstances(WasiComponent::Env &Env, IoHost &Host) {
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Out;
  auto Input = std::make_unique<TerminalInputInstance>();
  auto Output = std::make_unique<TerminalOutputInstance>();
  const auto *InputType = Input->findTypeResource("terminal-input");
  const auto *OutputType = Output->findTypeResource("terminal-output");
  Out.push_back(std::move(Input));
  Out.push_back(std::move(Output));
  Out.push_back(std::make_unique<EnvironmentInstance>(Env));
  Out.push_back(std::make_unique<ExitInstance>(Env));
  Out.push_back(std::make_unique<StdinInstance>(Host));
  Out.push_back(std::make_unique<StdoutInstance>(Host));
  Out.push_back(std::make_unique<StderrInstance>(Host));
  Out.push_back(std::make_unique<TerminalStdinInstance>(Env, InputType));
  Out.push_back(std::make_unique<TerminalStdoutInstance>(Env, OutputType));
  Out.push_back(std::make_unique<TerminalStderrInstance>(Env, OutputType));
  return Out;
}

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
