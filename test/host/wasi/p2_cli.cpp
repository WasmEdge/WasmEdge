// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- test/host/wasi/p2_cli.cpp - wasi:cli and wasi:random 0.2.12 hosts -===//

#include "common/component_variant.h"
#include "common/defines.h"
#include "host/wasi/component/env.h"
#include "host/wasi/p2/cli.h"
#include "host/wasi/p2/io.h"
#include "host/wasi/p2/random.h"
#include "hostcall.h"
#include "runtime/component/wit.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#if !WASMEDGE_OS_WINDOWS
#include <unistd.h>
#endif

namespace {

using namespace WasmEdge;
using namespace Host::WasiP2;
using HostTest::call;
using HostTest::u64Of;

TEST(WasiP2Cli, InstanceNamesAreTheInterfaceIds) {
  Host::WasiComponent::Env Env;
  Env.init({}, "prog", {}, {});
  IoHost Io(Env);
  auto IoInsts = makeIoInstances(Io);
  auto Insts = makeCliInstances(Env, Io);
  std::vector<std::string> Names;
  for (const auto &Inst : Insts) {
    Names.emplace_back(Inst->getComponentName());
  }
  EXPECT_EQ(Names, (std::vector<std::string>{
                       "wasi:cli/terminal-input@0.2.12",
                       "wasi:cli/terminal-output@0.2.12",
                       "wasi:cli/environment@0.2.12",
                       "wasi:cli/exit@0.2.12",
                       "wasi:cli/stdin@0.2.12",
                       "wasi:cli/stdout@0.2.12",
                       "wasi:cli/stderr@0.2.12",
                       "wasi:cli/terminal-stdin@0.2.12",
                       "wasi:cli/terminal-stdout@0.2.12",
                       "wasi:cli/terminal-stderr@0.2.12",
                   }));
  EXPECT_EQ(Insts[4]->findTypeResource("input-stream"), Io.InputType);
  EXPECT_EQ(Insts[5]->findTypeResource("output-stream"), Io.OutputType);
  EXPECT_NE(Insts[2]->findFunction("initial-cwd"), nullptr);
  auto Cwd = call(*Insts[2], "initial-cwd", {});
  ASSERT_TRUE(Cwd);
  EXPECT_FALSE(
      Runtime::Component::Wit<std::optional<std::string>>::from((*Cwd)[0]));
}

#if !WASMEDGE_OS_WINDOWS
TEST(WasiP2Cli, StandardStreamsReachThePipes) {
  int In[2];
  int Out[2];
  ASSERT_EQ(::pipe(In), 0);
  ASSERT_EQ(::pipe(Out), 0);
  Host::WasiComponent::Env Env;
  ASSERT_TRUE(Env.initWithFds({}, "prog", {}, {}, In[0], Out[1], Out[1]));
  IoHost Io(Env);
  auto IoInsts = makeIoInstances(Io);
  auto Insts = makeCliInstances(Env, Io);
  ASSERT_EQ(::write(In[1], "abc", 3), 3);
  ::close(In[1]);

  auto Stdin = call(*Insts[4], "get-stdin", {});
  ASSERT_TRUE(Stdin);
  const uint64_t InRep =
      Runtime::Component::Wit<Runtime::Component::Own<InputStream>>::from(
          (*Stdin)[0])
          .Rep;
  auto Read =
      call(*IoInsts[2], "[method]input-stream.blocking-read",
           {Runtime::Component::Wit<InputBorrow>::into(InputBorrow{InRep}),
            ComponentValVariant{uint64_t(10)}});
  ASSERT_TRUE(Read);
  auto Bytes =
      Runtime::Component::Wit<IoResult<std::vector<uint8_t>>>::from((*Read)[0]);
  ASSERT_TRUE(Bytes);
  EXPECT_EQ(*Bytes, (std::vector<uint8_t>{'a', 'b', 'c'}));

  auto Stdout = call(*Insts[5], "get-stdout", {});
  ASSERT_TRUE(Stdout);
  const uint64_t OutRep =
      Runtime::Component::Wit<Runtime::Component::Own<OutputStream>>::from(
          (*Stdout)[0])
          .Rep;
  auto Write =
      call(*IoInsts[2], "[method]output-stream.write",
           {Runtime::Component::Wit<OutputBorrow>::into(OutputBorrow{OutRep}),
            Runtime::Component::Wit<std::vector<uint8_t>>::into({'x', 'y'})});
  ASSERT_TRUE(Write);
  EXPECT_TRUE(Runtime::Component::Wit<IoResult<Runtime::Component::Unit>>::from(
      (*Write)[0]));
  char Buffer[4] = {};
  ASSERT_EQ(::read(Out[0], Buffer, 2), 2);
  EXPECT_EQ(std::string(Buffer, 2), "xy");
  auto Terminal = call(*Insts[7], "get-terminal-stdin", {});
  ASSERT_TRUE(Terminal);
  EXPECT_FALSE(
      std::get<OptionVal>(Runtime::Component::valComp((*Terminal)[0]).V)
          .Value.has_value());
  ::close(In[0]);
  ::close(Out[0]);
  ::close(Out[1]);
}
#endif

TEST(WasiP2Random, DrawsDifferentBytes) {
  auto Insts = makeRandomInstances();
  ASSERT_EQ(Insts.size(), 3u);
  EXPECT_EQ(Insts[0]->getComponentName(), "wasi:random/random@0.2.12");
  EXPECT_EQ(Insts[2]->getComponentName(), "wasi:random/insecure-seed@0.2.12");
  EXPECT_EQ(Insts[0]
                ->findFunction("get-random-bytes")
                ->getFuncType()
                .getParamList()[0]
                .getLabel(),
            "len");
  auto A =
      call(*Insts[0], "get-random-bytes", {ComponentValVariant{uint64_t(8)}});
  auto B =
      call(*Insts[0], "get-random-bytes", {ComponentValVariant{uint64_t(8)}});
  ASSERT_TRUE(A);
  ASSERT_TRUE(B);
  EXPECT_EQ(Runtime::Component::Wit<std::vector<uint8_t>>::from((*A)[0]).size(),
            8u);
  EXPECT_NE(Runtime::Component::Wit<std::vector<uint8_t>>::from((*A)[0]),
            Runtime::Component::Wit<std::vector<uint8_t>>::from((*B)[0]));
  auto U = call(*Insts[1], "get-insecure-random-u64", {});
  auto V = call(*Insts[1], "get-insecure-random-u64", {});
  ASSERT_TRUE(U);
  ASSERT_TRUE(V);
  EXPECT_NE(u64Of(U), u64Of(V));
  auto Seed = call(*Insts[2], "insecure-seed", {});
  ASSERT_TRUE(Seed);
  auto Pair =
      Runtime::Component::Wit<std::tuple<uint64_t, uint64_t>>::from((*Seed)[0]);
  EXPECT_NE(std::get<0>(Pair), std::get<1>(Pair));
}

} // namespace
