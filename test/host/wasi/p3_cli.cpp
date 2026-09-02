// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- test/host/wasi/p3_cli.cpp - wasi:cli@0.3.1 host functions ---------===//

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/defines.h"
#include "host/wasi/component/env.h"
#include "host/wasi/p3/cli.h"
#include "hostcall.h"
#include "runtime/component/wit.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#if !WASMEDGE_OS_WINDOWS
#include <unistd.h>
#endif

namespace {

using namespace WasmEdge;
using HostTest::call;

void initEnv(Host::WasiComponent::Env &Env, std::vector<std::string> Args,
             std::vector<std::string> Envs) {
  Env.init({}, "prog", Args, Envs);
}

TEST(WasiP3Cli, InstanceNamesAreTheInterfaceIds) {
  Host::WasiComponent::Env Env;
  auto Insts = Host::WasiP3::makeCliInstances(Env);
  std::vector<std::string> Names;
  for (const auto &Inst : Insts) {
    Names.emplace_back(Inst->getComponentName());
  }
  EXPECT_EQ(Names, (std::vector<std::string>{
                       "wasi:cli/terminal-input@0.3.1",
                       "wasi:cli/terminal-output@0.3.1",
                       "wasi:cli/types@0.3.1",
                       "wasi:cli/environment@0.3.1",
                       "wasi:cli/exit@0.3.1",
                       "wasi:cli/stdin@0.3.1",
                       "wasi:cli/stdout@0.3.1",
                       "wasi:cli/stderr@0.3.1",
                       "wasi:cli/terminal-stdin@0.3.1",
                       "wasi:cli/terminal-stdout@0.3.1",
                       "wasi:cli/terminal-stderr@0.3.1",
                   }));
}

TEST(WasiP3Cli, EnvironmentReportsArgumentsAndVariables) {
  Host::WasiComponent::Env Env;
  initEnv(Env, {"one", "two"}, {"A=1", "B=x=y", "C"});
  Host::WasiP3::EnvironmentInstance Inst(Env);
  auto Args = call(Inst, "get-arguments", {});
  ASSERT_TRUE(Args);
  EXPECT_EQ(Runtime::Component::Wit<std::vector<std::string>>::from((*Args)[0]),
            (std::vector<std::string>{"prog", "one", "two"}));
  auto Vars = call(Inst, "get-environment", {});
  ASSERT_TRUE(Vars);
  using Entry = std::tuple<std::string, std::string>;
  EXPECT_EQ(Runtime::Component::Wit<std::vector<Entry>>::from((*Vars)[0]),
            (std::vector<Entry>{{"A", "1"}, {"B", "x=y"}, {"C", ""}}));
  auto Cwd = call(Inst, "get-initial-cwd", {});
  ASSERT_TRUE(Cwd);
  EXPECT_FALSE(
      Runtime::Component::Wit<std::optional<std::string>>::from((*Cwd)[0]));
}

TEST(WasiP3Cli, ExitRecordsTheCodeAndTerminates) {
  Host::WasiComponent::Env Env;
  initEnv(Env, {}, {});
  Host::WasiP3::ExitInstance Inst(Env);
  using Status = Expected<Runtime::Component::Unit, Runtime::Component::Unit>;
  auto Ok =
      call(Inst, "exit",
           {Runtime::Component::Wit<Status>::into(Runtime::Component::Unit{})});
  ASSERT_FALSE(Ok);
  EXPECT_EQ(Ok.error(), ErrCode::Value::Terminated);
  EXPECT_EQ(Env.getExitCode(), 0u);
  auto Err = call(Inst, "exit",
                  {Runtime::Component::Wit<Status>::into(
                      Unexpected<Runtime::Component::Unit>({}))});
  ASSERT_FALSE(Err);
  EXPECT_EQ(Err.error(), ErrCode::Value::Terminated);
  EXPECT_EQ(Env.getExitCode(), 1u);
  auto Code = call(Inst, "exit-with-code", {ComponentValVariant{uint8_t(42)}});
  ASSERT_FALSE(Code);
  EXPECT_EQ(Code.error(), ErrCode::Value::Terminated);
  EXPECT_EQ(Env.getExitCode(), 42u);
}

TEST(WasiP3Cli, TypesExportTheErrorCodeEnum) {
  Host::WasiP3::CliTypesInstance Types;
  const auto *Ty = Types.findType("error-code");
  ASSERT_NE(Ty, nullptr);
  ASSERT_TRUE(Ty->getDefValType().isEnumTy());
  EXPECT_EQ(Ty->getDefValType().getEnum().Labels,
            (std::vector<std::string>{"io", "illegal-byte-sequence", "pipe"}));
}

TEST(WasiP3Cli, StdioDeclareTheirStreamShapes) {
  Host::WasiComponent::Env Env;
  Host::WasiP3::StdinInstance In(Env);
  auto *Read = In.findFunction("read-via-stream");
  ASSERT_NE(Read, nullptr);
  EXPECT_FALSE(Read->getFuncType().isAsync());
  ASSERT_EQ(Read->getFuncType().getResultList().size(), 1u);
  const auto *Tuple = *In.getType(
      Read->getFuncType().getResultList()[0].getValType().getTypeIndex());
  ASSERT_NE(Tuple, nullptr);
  ASSERT_TRUE(Tuple->getDefValType().isTupleTy());
  ASSERT_EQ(Tuple->getDefValType().getTuple().Types.size(), 2u);
  const auto *Stream =
      *In.getType(Tuple->getDefValType().getTuple().Types[0].getTypeIndex());
  ASSERT_TRUE(Stream->getDefValType().isStreamTy());
  EXPECT_EQ(*Stream->getDefValType().getStream().ValTy,
            ComponentValType(ComponentTypeCode::U8));
  const auto *Future =
      *In.getType(Tuple->getDefValType().getTuple().Types[1].getTypeIndex());
  ASSERT_TRUE(Future->getDefValType().isFutureTy());
  EXPECT_NE(In.findType("error-code"), nullptr);

  Host::WasiP3::StdoutInstance Out(Env);
  auto *Write = Out.findFunction("write-via-stream");
  ASSERT_NE(Write, nullptr);
  ASSERT_EQ(Write->getFuncType().getParamList().size(), 1u);
  EXPECT_EQ(Write->getFuncType().getParamList()[0].getLabel(), "data");
  const auto *Data = *Out.getType(
      Write->getFuncType().getParamList()[0].getValType().getTypeIndex());
  ASSERT_TRUE(Data->getDefValType().isStreamTy());
}

TEST(WasiP3Cli, TerminalInterfacesShareTheResourceTypes) {
  Host::WasiComponent::Env Env;
  auto Insts = Host::WasiP3::makeCliInstances(Env);
  const auto *InputType = Insts[0]->findTypeResource("terminal-input");
  ASSERT_NE(InputType, nullptr);
  EXPECT_EQ(Insts[8]->findTypeResource("terminal-input"), InputType);
  const auto *OutputType = Insts[1]->findTypeResource("terminal-output");
  ASSERT_NE(OutputType, nullptr);
  EXPECT_EQ(Insts[9]->findTypeResource("terminal-output"), OutputType);
  EXPECT_EQ(Insts[10]->findTypeResource("terminal-output"), OutputType);
  auto *Get = Insts[8]->findFunction("get-terminal-stdin");
  ASSERT_NE(Get, nullptr);
  const auto *Opt = *Insts[8]->getType(
      Get->getFuncType().getResultList()[0].getValType().getTypeIndex());
  ASSERT_TRUE(Opt->getDefValType().isOptionTy());
  const auto *Own =
      *Insts[8]->getType(Opt->getDefValType().getOption().ValTy.getTypeIndex());
  ASSERT_TRUE(Own->getDefValType().isOwnTy());
  EXPECT_EQ(*Insts[8]->getTypeResource(Own->getDefValType().getOwn().Idx),
            InputType);
}

#if !WASMEDGE_OS_WINDOWS
TEST(WasiP3Cli, TerminalsAreAbsentOnPipes) {
  int In[2];
  int Out[2];
  ASSERT_EQ(::pipe(In), 0);
  ASSERT_EQ(::pipe(Out), 0);
  Host::WasiComponent::Env Env;
  ASSERT_TRUE(Env.initWithFds({}, "prog", {}, {}, In[0], Out[1], Out[1]));
  auto Insts = Host::WasiP3::makeCliInstances(Env);
  for (auto [Idx, Name] :
       {std::pair{8, "get-terminal-stdin"}, std::pair{9, "get-terminal-stdout"},
        std::pair{10, "get-terminal-stderr"}}) {
    auto Res = call(*Insts[static_cast<size_t>(Idx)], Name, {});
    ASSERT_TRUE(Res) << Name;
    const auto &Opt =
        std::get<OptionVal>(Runtime::Component::valComp((*Res)[0]).V);
    EXPECT_FALSE(Opt.Value.has_value()) << Name;
  }
  EXPECT_FALSE(Env.isTerminal(0));
  EXPECT_FALSE(Env.isTerminal(1));
  ::close(In[0]);
  ::close(In[1]);
  ::close(Out[0]);
  ::close(Out[1]);
}
#endif

} // namespace
