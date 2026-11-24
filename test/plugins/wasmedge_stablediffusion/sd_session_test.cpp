// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "fake_backend.h"
#include "sd_session.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

namespace {

using namespace WasmEdge::Host::StableDiffusion;
using WasmEdge::Expected;
using WasmEdge::Span;

std::shared_ptr<Session> openSession(FakeBackend &B) {
  auto Ctx = B.open({}, {});
  return std::make_shared<Session>(std::move(*Ctx));
}

GenerationRequest req(uint32_t W = 64, uint32_t H = 64, uint32_t Batch = 1) {
  GenerationRequest Req;
  Req.Width = W;
  Req.Height = H;
  Req.BatchCount = Batch;
  return Req;
}

TEST(SDSessionTest, StartsReadyWithoutResults) {
  FakeBackend B;
  auto S = openSession(B);
  ResultInfo Info{};
  EXPECT_EQ(S->resultInfo(0, ResultFormat::Png, Info), ErrNo::NotFound);
  EXPECT_EQ(B.LiveContexts, 1);
}

TEST(SDSessionTest, GenerateStoresResults) {
  FakeBackend B;
  auto S = openSession(B);
  uint32_t Count = 0;
  ASSERT_EQ(S->generate(req(128, 64, 2), {}, {}, Count), ErrNo::Success);
  EXPECT_EQ(Count, 2U);
  ResultInfo Info{};
  ASSERT_EQ(S->resultInfo(1, ResultFormat::Rgb8, Info), ErrNo::Success);
  EXPECT_EQ(Info.Width, 128U);
  EXPECT_EQ(Info.Height, 64U);
  EXPECT_EQ(Info.Channels, 3U);
  EXPECT_EQ(Info.Size, 128U * 64U * 3U);
  ASSERT_EQ(S->resultInfo(0, ResultFormat::Png, Info), ErrNo::Success);
  EXPECT_GT(Info.Size, 8U);
  EXPECT_EQ(S->resultInfo(2, ResultFormat::Png, Info), ErrNo::NotFound);
}

TEST(SDSessionTest, FailedGenerationKeepsTheSessionAndDropsResults) {
  FakeBackend B;
  auto S = openSession(B);
  uint32_t Count = 0;
  ASSERT_EQ(S->generate(req(), {}, {}, Count), ErrNo::Success);
  B.OnGenerate = [](auto &, auto &, auto &) {
    return Unexpect(BackendFailure{ErrNo::LoadFailed, true});
  };
  const auto Live = OwnedImage::liveCount();
  EXPECT_EQ(S->generate(req(), {}, {}, Count), ErrNo::LoadFailed);
  EXPECT_EQ(OwnedImage::liveCount(), Live - 1);
  ResultInfo Info{};
  EXPECT_EQ(S->resultInfo(0, ResultFormat::Png, Info), ErrNo::NotFound);
  B.OnGenerate = nullptr;
  EXPECT_EQ(S->generate(req(), {}, {}, Count), ErrNo::Success);
}

TEST(SDSessionTest, AnUnusableContextPoisonsTheSession) {
  FakeBackend B;
  auto S = openSession(B);
  B.OnGenerate = [](auto &, auto &, auto &) {
    return Unexpect(BackendFailure{ErrNo::LoadFailed, false});
  };
  uint32_t Count = 0;
  EXPECT_EQ(S->generate(req(), {}, {}, Count), ErrNo::RuntimeError);
  const int Calls = B.Generates;
  B.OnGenerate = nullptr;
  EXPECT_EQ(S->generate(req(), {}, {}, Count), ErrNo::RuntimeError);
  EXPECT_EQ(B.Generates, Calls);
  EXPECT_EQ(B.LiveContexts, 1);
  ResultInfo Info{};
  EXPECT_EQ(S->resultInfo(0, ResultFormat::Png, Info), ErrNo::NotFound);
  uint32_t Size = 0;
  EXPECT_EQ(S->resultRead(0, ResultFormat::Rgb8, 1U << 20, Size,
                          [](Span<const uint8_t>) { return ErrNo::Success; }),
            ErrNo::NotFound);
  SessionTable Table;
  const auto Id = *Table.insert(S);
  S.reset();
  EXPECT_NE(Table.remove(Id), nullptr);
  EXPECT_EQ(B.LiveContexts, 0);
}

TEST(SDSessionTest, AnEmptyBatchIsAFailure) {
  FakeBackend B;
  auto S = openSession(B);
  B.OnGenerate = [](auto &, auto &, auto &) {
    return Expected<ImageBatch, BackendFailure>(ImageBatch{});
  };
  uint32_t Count = 0;
  EXPECT_EQ(S->generate(req(), {}, {}, Count), ErrNo::RuntimeError);
  ResultInfo Info{};
  EXPECT_EQ(S->resultInfo(0, ResultFormat::Rgb8, Info), ErrNo::NotFound);
  B.OnGenerate = nullptr;
  EXPECT_EQ(S->generate(req(), {}, {}, Count), ErrNo::Success);
  EXPECT_EQ(Count, 1U);
  EXPECT_EQ(S->resultInfo(0, static_cast<ResultFormat>(7), Info),
            ErrNo::InvalidArgument);
  uint32_t Size = 0;
  EXPECT_EQ(S->resultRead(0, static_cast<ResultFormat>(7), 1U << 20, Size,
                          [](Span<const uint8_t>) { return ErrNo::Success; }),
            ErrNo::InvalidArgument);
}

TEST(SDSessionTest, OnlyTheCodesOfTheStateTableReachTheGuest) {
  FakeBackend B;
  auto S = openSession(B);
  uint32_t Count = 0;
  for (const auto Code :
       {ErrNo::Success, ErrNo::InvalidEncoding, ErrNo::MissingMemory,
        ErrNo::Busy, ErrNo::BufferTooSmall, ErrNo::NotFound,
        ErrNo::NotReadOnly}) {
    B.OnGenerate = [Code](auto &, auto &, auto &) {
      return Unexpect(BackendFailure{Code, true});
    };
    EXPECT_EQ(S->generate(req(), {}, {}, Count), ErrNo::RuntimeError);
  }
  for (const auto Code :
       {ErrNo::InvalidArgument, ErrNo::LoadFailed, ErrNo::RuntimeError}) {
    B.OnGenerate = [Code](auto &, auto &, auto &) {
      return Unexpect(BackendFailure{Code, true});
    };
    EXPECT_EQ(S->generate(req(), {}, {}, Count), Code);
  }
  B.OnGenerate = nullptr;
  EXPECT_EQ(S->generate(req(), {}, {}, Count), ErrNo::Success);
}

TEST(SDSessionTest, ABatchWithAnImageThatIsNotRgbIsAFailure) {
  FakeBackend B;
  auto S = openSession(B);
  const auto Live = OwnedImage::liveCount();
  B.OnGenerate = [](auto &, auto &, auto &) {
    ImageBatch Batch = solidImages(1, 64, 64);
    Batch.push_back(std::move(
        *OwnedImage::copyOf(std::vector<uint8_t>(64 * 64), 64, 64, 1)));
    return Expected<ImageBatch, BackendFailure>(std::move(Batch));
  };
  uint32_t Count = 0;
  EXPECT_EQ(S->generate(req(), {}, {}, Count), ErrNo::RuntimeError);
  EXPECT_EQ(OwnedImage::liveCount(), Live);
  ResultInfo Info{};
  EXPECT_EQ(S->resultInfo(0, ResultFormat::Rgb8, Info), ErrNo::NotFound);
  B.OnGenerate = nullptr;
  EXPECT_EQ(S->generate(req(), {}, {}, Count), ErrNo::Success);
}

TEST(SDSessionTest, PreviousResultsAreDroppedBeforeTheBackendRuns) {
  FakeBackend B;
  auto S = openSession(B);
  const auto Empty = OwnedImage::liveCount();
  uint32_t Count = 0;
  ASSERT_EQ(S->generate(req(64, 64, 2), {}, {}, Count), ErrNo::Success);
  EXPECT_EQ(OwnedImage::liveCount(), Empty + 2);
  uint64_t SeenInside = UINT64_MAX;
  B.OnGenerate = [&](auto &Req, auto &, auto &) {
    SeenInside = OwnedImage::liveCount();
    return Expected<ImageBatch, BackendFailure>(
        solidImages(1, Req.Width, Req.Height));
  };
  ASSERT_EQ(S->generate(req(), {}, {}, Count), ErrNo::Success);
  EXPECT_EQ(SeenInside, Empty);
}

TEST(SDSessionTest, ThePngIsEncodedOnceAndKept) {
  FakeBackend B;
  auto S = openSession(B);
  uint32_t Count = 0;
  ASSERT_EQ(S->generate(req(), {}, {}, Count), ErrNo::Success);
  const uint8_t *First = nullptr;
  const uint8_t *Second = nullptr;
  uint32_t Size = 0;
  ASSERT_EQ(S->resultRead(0, ResultFormat::Png, UINT32_MAX, Size,
                          [&](Span<const uint8_t> Bytes) {
                            First = Bytes.data();
                            return ErrNo::Success;
                          }),
            ErrNo::Success);
  ASSERT_EQ(S->resultRead(0, ResultFormat::Png, UINT32_MAX, Size,
                          [&](Span<const uint8_t> Bytes) {
                            Second = Bytes.data();
                            return ErrNo::Success;
                          }),
            ErrNo::Success);
  EXPECT_NE(First, nullptr);
  EXPECT_EQ(First, Second);
}

TEST(SDSessionTest, AReadWaitsForARunningGeneration) {
  FakeBackend B;
  auto S = openSession(B);
  uint32_t Count = 0;
  ASSERT_EQ(S->generate(req(), {}, {}, Count), ErrNo::Success);
  std::promise<void> Entered, Release;
  B.OnGenerate = [&](auto &Req, auto &, auto &) {
    Entered.set_value();
    Release.get_future().wait();
    return Expected<ImageBatch, BackendFailure>(
        solidImages(1, Req.Width, Req.Height));
  };
  auto Running = std::async(std::launch::async, [&] {
    uint32_t Mine = 0;
    return S->generate(req(), {}, {}, Mine);
  });
  Entered.get_future().wait();
  auto Reader = std::async(std::launch::async, [&] {
    uint32_t Size = 0;
    return S->resultRead(0, ResultFormat::Rgb8, UINT32_MAX, Size,
                         [](Span<const uint8_t>) { return ErrNo::Success; });
  });
  EXPECT_EQ(Reader.wait_for(std::chrono::milliseconds(100)),
            std::future_status::timeout);
  Release.set_value();
  EXPECT_EQ(Running.get(), ErrNo::Success);
  EXPECT_EQ(Reader.get(), ErrNo::Success);
}

TEST(SDSessionTest, ResultReadCopiesOnlyWhenTheBufferFits) {
  FakeBackend B;
  auto S = openSession(B);
  uint32_t Count = 0;
  ASSERT_EQ(S->generate(req(), {}, {}, Count), ErrNo::Success);
  const uint32_t Want = 64U * 64U * 3U;
  uint32_t Size = 0;
  int Copies = 0;
  const auto Copy = [&](Span<const uint8_t> Bytes) {
    ++Copies;
    EXPECT_EQ(Bytes.size(), Want);
    return ErrNo::Success;
  };
  EXPECT_EQ(S->resultRead(0, ResultFormat::Rgb8, Want - 1, Size, Copy),
            ErrNo::BufferTooSmall);
  EXPECT_EQ(Size, Want);
  EXPECT_EQ(Copies, 0);
  EXPECT_EQ(S->resultRead(0, ResultFormat::Rgb8, Want, Size, Copy),
            ErrNo::Success);
  EXPECT_EQ(Copies, 1);
  EXPECT_EQ(
      S->resultRead(0, ResultFormat::Rgb8, Want, Size,
                    [](Span<const uint8_t>) { return ErrNo::MissingMemory; }),
      ErrNo::MissingMemory);
  EXPECT_EQ(S->resultRead(9, ResultFormat::Rgb8, Want, Size, Copy),
            ErrNo::NotFound);
}

TEST(SDSessionTest, ASecondCallWaitsForTheFirst) {
  FakeBackend B;
  auto S = openSession(B);
  std::promise<void> Entered, Release;
  B.OnGenerate = [&](auto &Req, auto &, auto &) {
    Entered.set_value();
    Release.get_future().wait();
    return Expected<ImageBatch, BackendFailure>(
        solidImages(1, Req.Width, Req.Height));
  };
  uint32_t Count = 0;
  auto First = std::async(std::launch::async,
                          [&] { return S->generate(req(), {}, {}, Count); });
  Entered.get_future().wait();
  auto Second = std::async(std::launch::async, [&] {
    ResultInfo Info{};
    return S->resultInfo(0, ResultFormat::Rgb8, Info);
  });
  EXPECT_EQ(Second.wait_for(std::chrono::milliseconds(100)),
            std::future_status::timeout);
  Release.set_value();
  EXPECT_EQ(First.get(), ErrNo::Success);
  EXPECT_EQ(Second.get(), ErrNo::Success);
}

TEST(SDSessionTest, GenerateDecodesItsInputsWhenItHasTheLock) {
  FakeBackend B;
  auto S = openSession(B);
  std::promise<void> Entered, Release;
  std::atomic<int> Generations{0};
  B.OnGenerate = [&](auto &Req, auto &, auto &) {
    if (Generations++ == 0) {
      Entered.set_value();
      Release.get_future().wait();
    }
    return Expected<ImageBatch, BackendFailure>(
        solidImages(1, Req.Width, Req.Height));
  };
  auto First = std::async(std::launch::async, [&] {
    uint32_t Count = 0;
    return S->generate(req(), {}, {}, Count);
  });
  Entered.get_future().wait();
  std::atomic<bool> Decoded{false};
  auto Second = std::async(std::launch::async, [&] {
    uint32_t Count = 0;
    return S->generate(
        req(), {},
        [&] {
          Decoded = true;
          return Expected<GenerationInputs, ErrNo>(GenerationInputs());
        },
        Count);
  });
  EXPECT_EQ(Second.wait_for(std::chrono::milliseconds(100)),
            std::future_status::timeout);
  EXPECT_FALSE(Decoded);
  Release.set_value();
  EXPECT_EQ(First.get(), ErrNo::Success);
  EXPECT_EQ(Second.get(), ErrNo::Success);
  EXPECT_TRUE(Decoded);
}

TEST(SDSessionTest, AFailedDecodeKeepsTheResults) {
  FakeBackend B;
  auto S = openSession(B);
  uint32_t Count = 0;
  ASSERT_EQ(S->generate(req(), {}, {}, Count), ErrNo::Success);
  const int Calls = B.Generates;
  EXPECT_EQ(S->generate(
                req(), {},
                []() -> Expected<GenerationInputs, ErrNo> {
                  return WasmEdge::Unexpected<ErrNo>(ErrNo::InvalidEncoding);
                },
                Count),
            ErrNo::InvalidEncoding);
  EXPECT_EQ(B.Generates, Calls);
  ResultInfo Info{};
  EXPECT_EQ(S->resultInfo(0, ResultFormat::Rgb8, Info), ErrNo::Success);
}

TEST(SDSessionTest, CloseDuringGenerationFreesAtTheLastReference) {
  FakeBackend B;
  SessionTable Table;
  const auto Id = *Table.insert(openSession(B));
  std::promise<void> Entered, Release;
  B.OnGenerate = [&](auto &Req, auto &, auto &) {
    Entered.set_value();
    Release.get_future().wait();
    return Expected<ImageBatch, BackendFailure>(
        solidImages(1, Req.Width, Req.Height));
  };
  auto Running = std::async(std::launch::async, [&] {
    auto S = Table.get(Id);
    uint32_t Count = 0;
    return S->generate(req(), {}, {}, Count);
  });
  Entered.get_future().wait();
  EXPECT_NE(Table.remove(Id), nullptr);
  EXPECT_EQ(Table.get(Id), nullptr);
  EXPECT_EQ(B.LiveContexts, 1);
  Release.set_value();
  EXPECT_EQ(Running.get(), ErrNo::Success);
  EXPECT_EQ(B.LiveContexts, 0);
}

TEST(SDSessionTest, ASessionKeepsItsPlaceUntilItsContextIsFreed) {
  // Notes how many places are taken at the time it is freed.
  class Watching : public BackendContext {
  public:
    Watching(const std::atomic<uint32_t> &Taken, uint32_t &AtFree) noexcept
        : Taken(Taken), AtFree(AtFree) {}
    ~Watching() noexcept override { AtFree = Taken.load(); }
    const Capabilities &capabilities() const noexcept override { return Caps; }
    Expected<ImageBatch, BackendFailure>
    generate(const GenerationRequest &, const RequestFiles &,
             const GenerationInputs &) noexcept override {
      return ImageBatch();
    }

  private:
    const std::atomic<uint32_t> &Taken;
    uint32_t &AtFree;
    const Capabilities Caps;
  };
  std::atomic<uint32_t> Taken{0};
  uint32_t AtFree = 0;
  {
    const Session S(std::make_unique<Watching>(Taken, AtFree),
                    takeSlot(Taken, 1));
    EXPECT_EQ(Taken.load(), 1U);
    EXPECT_EQ(takeSlot(Taken, 1), nullptr);
  }
  EXPECT_EQ(AtFree, 1U);
  EXPECT_EQ(Taken.load(), 0U);
}

TEST(SDSessionTest, TheTableFreesEverySessionAndNeverReusesAnId) {
  FakeBackend B;
  const auto Images = OwnedImage::liveCount();
  {
    SessionTable Table;
    const auto A = *Table.insert(openSession(B));
    uint32_t Count = 0;
    ASSERT_EQ(Table.get(A)->generate(req(), {}, {}, Count), ErrNo::Success);
    Table.remove(A);
    const auto C = *Table.insert(openSession(B));
    EXPECT_NE(A, C);
    Table.insert(openSession(B));
    EXPECT_EQ(B.LiveContexts, 2);
  }
  EXPECT_EQ(B.LiveContexts, 0);
  EXPECT_EQ(OwnedImage::liveCount(), Images);
}

TEST(SDSessionTest, ChecksRequestsAgainstCapabilities) {
  auto Control = req();
  Control.ControlImage = 0;
  EXPECT_EQ(checkAgainst({}, Control), ErrNo::InvalidArgument);
  EXPECT_EQ(checkAgainst({true, false, 0}, Control), ErrNo::Success);
  auto Up = req(128, 64);
  Up.UpscaleRepeats = 3;
  EXPECT_EQ(checkAgainst({}, Up), ErrNo::InvalidArgument);
  EXPECT_EQ(checkAgainst({false, true, 4}, Up), ErrNo::Success);
  Up.Width = 192;
  EXPECT_EQ(checkAgainst({false, true, 4}, Up), ErrNo::InvalidArgument);
  auto Wide = req(2048, 2048);
  Wide.UpscaleRepeats = 1;
  EXPECT_EQ(checkAgainst({false, true, 4}, Wide), ErrNo::InvalidArgument);
  EXPECT_EQ(checkAgainst({false, true, 2}, Wide), ErrNo::Success);
  auto Side = req(4096, 64);
  Side.UpscaleRepeats = 1;
  EXPECT_EQ(checkAgainst({false, true, 4}, Side), ErrNo::InvalidArgument);
  EXPECT_EQ(checkAgainst({false, true, 2}, Side), ErrNo::Success);
  EXPECT_EQ(checkAgainst({false, true, 0}, Side), ErrNo::InvalidArgument);
  EXPECT_EQ(checkAgainst({false, false, 2}, Side), ErrNo::InvalidArgument);
  auto WithLora = req();
  WithLora.Loras = {{"/l/a.safetensors", 1.0F}};
  EXPECT_EQ(checkAgainst({}, WithLora), ErrNo::InvalidArgument);
  EXPECT_EQ(checkAgainst({true, true, 4, false}, WithLora),
            ErrNo::InvalidArgument);
  EXPECT_EQ(checkAgainst({false, false, 0, true}, WithLora), ErrNo::Success);
}

} // namespace
