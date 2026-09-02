// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- test/host/wasi/p2_io.cpp - wasi:io and wasi:clocks 0.2.12 hosts ---===//

#include "common/component_variant.h"
#include "common/defines.h"
#include "host/wasi/component/env.h"
#include "host/wasi/p2/clocks.h"
#include "host/wasi/p2/io.h"
#include "hostcall.h"
#include "runtime/component/wit.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#if !WASMEDGE_OS_WINDOWS
#include <unistd.h>
#endif

namespace {

using namespace WasmEdge;
using namespace Host::WasiP2;
using namespace std::chrono_literals;
using HostTest::call;
using HostTest::u64Of;

class WasiP2Io : public testing::Test {
protected:
  void SetUp() override {
#if !WASMEDGE_OS_WINDOWS
    ASSERT_EQ(::pipe(In), 0);
    ASSERT_EQ(::pipe(Out), 0);
    ASSERT_TRUE(Env.initWithFds({}, "prog", {}, {}, In[0], Out[1], Out[1]));
#else
    Env.init({}, "prog", {}, {});
#endif
    Io = std::make_unique<IoHost>(Env);
    Insts = makeIoInstances(*Io);
    Streams = Insts[2].get();
    Poll = Insts[1].get();
  }
  void TearDown() override {
#if !WASMEDGE_OS_WINDOWS
    for (int Fd : {In[0], In[1], Out[0], Out[1]}) {
      if (Fd >= 0) {
        ::close(Fd);
      }
    }
#endif
  }
  ComponentValVariant input(uint64_t Rep) {
    return Runtime::Component::Wit<InputBorrow>::into(InputBorrow{Rep});
  }
  ComponentValVariant output(uint64_t Rep) {
    return Runtime::Component::Wit<OutputBorrow>::into(OutputBorrow{Rep});
  }
  ComponentValVariant pollable(uint64_t Rep) {
    return Runtime::Component::Wit<PollableBorrow>::into(PollableBorrow{Rep});
  }

  int In[2] = {-1, -1};
  int Out[2] = {-1, -1};
  Host::WasiComponent::Env Env;
  std::unique_ptr<IoHost> Io;
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Insts;
  Runtime::Instance::ComponentInstance *Streams = nullptr;
  Runtime::Instance::ComponentInstance *Poll = nullptr;
};

TEST_F(WasiP2Io, InstancesAndTypes) {
  EXPECT_EQ(Insts[0]->getComponentName(), "wasi:io/error@0.2.12");
  EXPECT_EQ(Insts[1]->getComponentName(), "wasi:io/poll@0.2.12");
  EXPECT_EQ(Insts[2]->getComponentName(), "wasi:io/streams@0.2.12");
  EXPECT_EQ(Io->ErrorType, Insts[0]->findTypeResource("error"));
  EXPECT_EQ(Io->PollableType, Insts[1]->findTypeResource("pollable"));
  EXPECT_EQ(Streams->findTypeResource("error"), Io->ErrorType);
  EXPECT_EQ(Streams->findTypeResource("pollable"), Io->PollableType);
  EXPECT_EQ(Streams->findTypeResource("input-stream"), Io->InputType);
  EXPECT_EQ(Streams->findTypeResource("output-stream"), Io->OutputType);
  const auto *Err = Streams->findType("stream-error");
  ASSERT_NE(Err, nullptr);
  ASSERT_TRUE(Err->getDefValType().isVariantTy());
  EXPECT_EQ(Err->getDefValType().getVariant().Cases[0].first,
            "last-operation-failed");
  EXPECT_EQ(Err->getDefValType().getVariant().Cases[1].first, "closed");
  EXPECT_NE(Streams->findFunction("[method]output-stream.blocking-splice"),
            nullptr);
}

#if !WASMEDGE_OS_WINDOWS
TEST_F(WasiP2Io, StandardStreamsOverPipes) {
  auto Stdin = std::make_shared<NodeInputStream>(*Io, Env.stdioNode(0),
                                                 NodeInputStream::Mode::Fd);
  const uint64_t InRep = Io->inputs().add(Stdin);
  auto Stdout = std::make_shared<NodeOutputStream>(*Io, Env.stdioNode(1),
                                                   NodeOutputStream::Mode::Fd);
  const uint64_t OutRep = Io->outputs().add(Stdout);

  // Nothing written yet: a non-blocking read is empty and the pollable is
  // not ready.
  auto Empty = call(*Streams, "[method]input-stream.read",
                    {input(InRep), ComponentValVariant{uint64_t(16)}});
  ASSERT_TRUE(Empty);
  auto Bytes = Runtime::Component::Wit<IoResult<std::vector<uint8_t>>>::from(
      (*Empty)[0]);
  ASSERT_TRUE(Bytes);
  EXPECT_TRUE(Bytes->empty());
  auto Sub = call(*Streams, "[method]input-stream.subscribe", {input(InRep)});
  ASSERT_TRUE(Sub);
  const uint64_t PollRep =
      Runtime::Component::Wit<Runtime::Component::Own<PollSource>>::from(
          (*Sub)[0])
          .Rep;
  auto NotReady = call(*Poll, "[method]pollable.ready", {pollable(PollRep)});
  ASSERT_TRUE(NotReady);
  EXPECT_FALSE(std::get<bool>((*NotReady)[0]));

  // A writer thread wakes the blocking read.
  std::thread Writer([this]() {
    std::this_thread::sleep_for(20ms);
    ASSERT_EQ(::write(In[1], "hello", 5), 5);
  });
  auto Got = call(*Streams, "[method]input-stream.blocking-read",
                  {input(InRep), ComponentValVariant{uint64_t(16)}});
  Writer.join();
  ASSERT_TRUE(Got);
  auto Hello =
      Runtime::Component::Wit<IoResult<std::vector<uint8_t>>>::from((*Got)[0]);
  ASSERT_TRUE(Hello);
  EXPECT_EQ(*Hello, (std::vector<uint8_t>{'h', 'e', 'l', 'l', 'o'}));

  // Writing goes straight to the pipe; check-write reports a budget.
  auto Budget =
      call(*Streams, "[method]output-stream.check-write", {output(OutRep)});
  ASSERT_TRUE(Budget);
  auto Allowed =
      Runtime::Component::Wit<IoResult<uint64_t>>::from((*Budget)[0]);
  ASSERT_TRUE(Allowed);
  EXPECT_GT(*Allowed, 0u);
  auto Written =
      call(*Streams, "[method]output-stream.blocking-write-and-flush",
           {output(OutRep), Runtime::Component::Wit<std::vector<uint8_t>>::into(
                                {'p', 'o', 'n', 'g'})});
  ASSERT_TRUE(Written);
  EXPECT_TRUE(Runtime::Component::Wit<IoResult<Runtime::Component::Unit>>::from(
      (*Written)[0]));
  char Buffer[8] = {};
  ASSERT_EQ(::read(Out[0], Buffer, 4), 4);
  EXPECT_EQ(std::string(Buffer, 4), "pong");

  // End of input: the read reports closed and the pollable is ready.
  ::close(In[1]);
  In[1] = -1;
  auto End = call(*Streams, "[method]input-stream.blocking-read",
                  {input(InRep), ComponentValVariant{uint64_t(16)}});
  ASSERT_TRUE(End);
  auto Closed =
      Runtime::Component::Wit<IoResult<std::vector<uint8_t>>>::from((*End)[0]);
  ASSERT_FALSE(Closed);
  EXPECT_TRUE(Closed.error().Closed);
  auto Ready = call(*Poll, "[method]pollable.ready", {pollable(PollRep)});
  ASSERT_TRUE(Ready);
  EXPECT_TRUE(std::get<bool>((*Ready)[0]));
}
#endif

TEST_F(WasiP2Io, PollWaitsForTheEarliestDeadline) {
  auto Clocks = makeClocksInstances(*Io);
  ASSERT_EQ(Clocks.size(), 2u);
  EXPECT_EQ(Clocks[0]->getComponentName(), "wasi:clocks/wall-clock@0.2.12");
  EXPECT_EQ(Clocks[1]->getComponentName(),
            "wasi:clocks/monotonic-clock@0.2.12");
  auto Now = call(*Clocks[1], "now", {});
  ASSERT_TRUE(Now);
  auto Later = call(*Clocks[1], "subscribe-duration",
                    {ComponentValVariant{uint64_t(500000000)}});
  auto Soon = call(*Clocks[1], "subscribe-instant",
                   {ComponentValVariant{u64Of(Now) + 20000000}});
  ASSERT_TRUE(Later);
  ASSERT_TRUE(Soon);
  const uint64_t LaterRep =
      Runtime::Component::Wit<Runtime::Component::Own<PollSource>>::from(
          (*Later)[0])
          .Rep;
  const uint64_t SoonRep =
      Runtime::Component::Wit<Runtime::Component::Own<PollSource>>::from(
          (*Soon)[0])
          .Rep;
  const auto T0 = std::chrono::steady_clock::now();
  auto Res = call(*Poll, "poll",
                  {Runtime::Component::Wit<std::vector<PollableBorrow>>::into(
                      {PollableBorrow{LaterRep}, PollableBorrow{SoonRep}})});
  const auto Elapsed = std::chrono::steady_clock::now() - T0;
  ASSERT_TRUE(Res);
  EXPECT_EQ(Runtime::Component::Wit<std::vector<uint32_t>>::from((*Res)[0]),
            (std::vector<uint32_t>{1}));
  EXPECT_GE(Elapsed, 20ms);
  EXPECT_LT(Elapsed, 400ms);
  auto Block = call(*Poll, "[method]pollable.block", {pollable(SoonRep)});
  ASSERT_TRUE(Block);

  auto Wall = call(*Clocks[0], "now", {});
  ASSERT_TRUE(Wall);
  EXPECT_GT(Runtime::Component::Wit<Datetime>::from((*Wall)[0]).Seconds,
            1700000000u);
  auto Res2 = call(*Clocks[0], "resolution", {});
  ASSERT_TRUE(Res2);
  EXPECT_GT(Runtime::Component::Wit<Datetime>::from((*Res2)[0]).Nanoseconds +
                Runtime::Component::Wit<Datetime>::from((*Res2)[0]).Seconds,
            0u);
}

TEST_F(WasiP2Io, ErrorsCarryTheirMessage) {
  const uint64_t Rep = Io->errors().add(
      std::make_shared<IoError>(IoError{__WASI_ERRNO_IO, "boom"}));
  auto Res =
      call(*Insts[0], "[method]error.to-debug-string",
           {Runtime::Component::Wit<ErrorBorrow>::into(ErrorBorrow{Rep})});
  ASSERT_TRUE(Res);
  EXPECT_EQ(Runtime::Component::Wit<std::string>::from((*Res)[0]), "boom");
  auto Val =
      Io->errorValue(StreamError{false, IoError{__WASI_ERRNO_ACCES, "x"}});
  EXPECT_FALSE(Val.Closed);
  EXPECT_EQ(Io->errors().get(Val.ErrorRep)->Errno, __WASI_ERRNO_ACCES);
}

} // namespace
