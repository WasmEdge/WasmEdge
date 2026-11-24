// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "fake_backend.h"
#include "sd_abi.h"
#include "sd_backend.h"
#include "sd_guest_file.h"
#include "sd_image.h"
#include "sd_module.h"
#include "test_support.h"

#include "common/expected.h"
#include "common/spdlog.h"
#include "host/mock/wasmedge_stablediffusion_module.h"
#include "runtime/instance/function.h"
#include "runtime/instance/module.h"

#include <gtest/gtest.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/ostream_sink.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

#define E(x) static_cast<uint32_t>(ErrNo::x)

namespace {

using namespace std::literals;
using namespace WasmEdge::Host::StableDiffusion;
using WasmEdge::Expected;
namespace Host = WasmEdge::Host;

constexpr std::string_view kModelConfig = R"({"model_path":"/models/m.bin"})"sv;

uint32_t pngSize(uint32_t W, uint32_t H) {
  return static_cast<uint32_t>(png(W, H).size());
}

// Waits up to five seconds and returns the first time from which File does not
// count as changed.
std::chrono::system_clock::time_point
settledAfter(const std::filesystem::path &File) {
  const auto GiveUp =
      std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (true) {
    const auto Now = std::chrono::system_clock::now();
    struct stat St{};
    if (::stat(File.c_str(), &St) == 0 &&
        !detail::changedSince(detail::changeTimeOf(St), detail::stampOf(Now))) {
      return Now;
    }
    if (std::chrono::steady_clock::now() > GiveUp) {
      ADD_FAILURE() << File << " did not settle within 5 seconds";
      return Now;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

class SDFuncTest : public ::testing::Test {
protected:
  SDFuncTest() { Fake->Caps.Lora = true; }
  void SetUp() override {
    namespace fs = std::filesystem;
    LoggerBefore = spdlog::default_logger();
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(
        "sd-func-quiet", std::make_shared<spdlog::sinks::null_sink_mt>()));
    Root = fs::canonical(fs::temp_directory_path()) /
           ("wasmedge-sd-func-" + std::to_string(::getpid()));
    fs::remove_all(Root);
    fs::create_directories(Root / "models");
    fs::create_directories(Root / "work");
    std::ofstream(Root / "models" / "m.bin", std::ios::binary) << "MODEL";
    std::ofstream(Root / "work" / "w.bin", std::ios::binary) << "WORK";
    G.withPreopens({"/models:" + (Root / "models").string() + ":readonly",
                    "/work:" + (Root / "work").string()});
  }
  void TearDown() override {
    std::error_code Error;
    std::filesystem::remove_all(Root, Error);
    spdlog::set_default_logger(LoggerBefore);
    spdlog::drop("sd-func-quiet");
  }

  // Calls session_open of Module with Config. The id goes to IdPtr.
  uint32_t openOn(WasmEdge::Runtime::Instance::ModuleInstance &Module,
                  std::string_view Config, uint32_t IdPtr) {
    const auto Text = G.put(Config);
    return G.call(Module, "session_open",
                  {Text, static_cast<uint32_t>(Config.size()), IdPtr});
  }
  uint32_t open() {
    const auto IdPtr = G.put(std::string(4, '\0'));
    EXPECT_EQ(openOn(Mod, kModelConfig, IdPtr), E(Success));
    return G.u32(IdPtr);
  }
  uint32_t generate(uint32_t Id, std::string_view Req, uint32_t Table = 0,
                    uint32_t Blobs = 0) {
    const auto Text = G.put(Req);
    CountSlot = G.put(std::string(4, '\0'));
    return G.call(
        Mod, "generate",
        {Id, Text, static_cast<uint32_t>(Req.size()), Table, Blobs, CountSlot});
  }
  uint32_t count() { return G.u32(CountSlot); }
  void generateOnce(uint32_t Id) {
    ASSERT_EQ(generate(Id, R"({"prompt":"a","width":64,"height":64})"sv),
              E(Success));
  }

  std::shared_ptr<FakeBackend> Fake = std::make_shared<FakeBackend>();
  // The creation time is one hour from now. /work is writable and on the same
  // file system as the files that SetUp writes. With the real creation time,
  // those files would count as changed.
  Host::SDModule Mod{Fake,
                     std::chrono::system_clock::now() + std::chrono::hours(1)};
  Guest G;
  std::filesystem::path Root;
  uint32_t CountSlot = 0;
  std::shared_ptr<spdlog::logger> LoggerBefore;
};

TEST_F(SDFuncTest, OpenGenerateReadClose) {
  const auto Id = open();
  EXPECT_EQ(Fake->LastSessionFiles.Model, (Root / "models" / "m.bin").string());
  EXPECT_EQ(Fake->LastSessionConfig.ModelPath, ""sv);
  ASSERT_EQ(
      generate(Id, R"({"prompt":"a","width":64,"height":64,)"
                   R"("loras":[{"path":"/models/m.bin","multiplier":0.5}]})"sv),
      E(Success));
  EXPECT_EQ(count(), 1U);
  ASSERT_EQ(Fake->LastGenerationRequest.Loras.size(), 1U);
  EXPECT_EQ(Fake->LastGenerationRequest.Loras[0].Path, ""sv);
  EXPECT_EQ(Fake->LastGenerationRequest.Loras[0].Multiplier, 0.5F);
  ASSERT_EQ(Fake->LastRequestFiles.Loras.size(), 1U);
  EXPECT_EQ(Fake->LastRequestFiles.Loras[0],
            (Root / "models" / "m.bin").string());
  const auto Info = G.put(std::string(16, '\0'));
  ASSERT_EQ(G.call(Mod, "result_info", {Id, UINT32_C(0), UINT32_C(1), Info}),
            E(Success));
  EXPECT_EQ(G.u32(Info), 64U);
  EXPECT_EQ(G.u32(Info + 12), 64U * 64U * 3U);
  const auto Buf = G.put(std::string(64 * 64 * 3, '\x55'));
  const auto Written = G.put(std::string(4, '\0'));
  ASSERT_EQ(G.call(Mod, "result_read",
                   {Id, UINT32_C(0), UINT32_C(1), Buf, UINT32_C(64 * 64 * 3),
                    Written}),
            E(Success));
  EXPECT_EQ(G.u32(Written), 64U * 64U * 3U);
  EXPECT_EQ(G.byte(Buf), 0x80U);
  EXPECT_EQ(G.byte(Buf + 64 * 64 * 3 - 1), 0x80U);
  EXPECT_EQ(G.call(Mod, "session_close", {Id}), E(Success));
  EXPECT_EQ(Fake->LiveContexts, 0);

  const auto AllPaths =
      R"({"model_path":"/models/m.bin","diffusion_model_path":"/models/m.bin",)"
      R"("clip_l_path":"/models/m.bin","clip_g_path":"/models/m.bin",)"
      R"("t5xxl_path":"/models/m.bin","vae_path":"/models/m.bin",)"
      R"("taesd_path":"/models/m.bin","control_net_path":"/models/m.bin",)"
      R"("upscaler_model_path":"/models/m.bin",)"
      R"("embeddings":[{"name":"e1","path":"/models/m.bin"}]})"sv;
  ASSERT_EQ(openOn(Mod, AllPaths, G.put(std::string(4, '\0'))), E(Success));
  const auto &Config = Fake->LastSessionConfig;
  for (const auto *GuestPath :
       {&Config.ModelPath, &Config.DiffusionModelPath, &Config.ClipLPath,
        &Config.ClipGPath, &Config.T5xxlPath, &Config.VaePath,
        &Config.TaesdPath, &Config.ControlNetPath, &Config.UpscalerModelPath}) {
    EXPECT_EQ(*GuestPath, ""sv);
  }
  ASSERT_EQ(Config.Embeddings.size(), 1U);
  EXPECT_EQ(Config.Embeddings[0].Name, "e1"sv);
  EXPECT_EQ(Config.Embeddings[0].Path, ""sv);
  const auto &Files = Fake->LastSessionFiles;
  for (const auto *HostPath :
       {&Files.Model, &Files.DiffusionModel, &Files.ClipL, &Files.ClipG,
        &Files.T5xxl, &Files.Vae, &Files.Taesd, &Files.ControlNet,
        &Files.Upscaler}) {
    EXPECT_EQ(*HostPath, (Root / "models" / "m.bin").string());
  }
  ASSERT_EQ(Files.Embeddings.size(), 1U);
  EXPECT_EQ(Files.Embeddings[0], (Root / "models" / "m.bin").string());
}

TEST_F(SDFuncTest, AFailedOpenPublishesNothing) {
  const auto IdPtr = G.put(std::string(4, '\x7f'));
  const auto Run = [&](std::string_view Config) {
    const auto P = G.put(Config);
    return G.call(Mod, "session_open",
                  {P, static_cast<uint32_t>(Config.size()), IdPtr});
  };
  const auto Opens = static_cast<int>(Fake->Opens);
  EXPECT_EQ(G.call(Mod, "session_open",
                   {UINT32_C(0xffffffff), Limits::MaxTextBytes + 1, IdPtr}),
            E(InvalidArgument));
  EXPECT_EQ(
      G.call(Mod, "session_open", {G.memorySize() - 2, UINT32_C(10), IdPtr}),
      E(MissingMemory));
  EXPECT_EQ(openOn(Mod, kModelConfig, G.memorySize() - 3), E(MissingMemory));
  EXPECT_EQ(Fake->Opens, Opens);
  EXPECT_EQ(Run("{"sv), E(InvalidEncoding));
  EXPECT_EQ(Run(R"({"model_path":"/models/m.bin","x":1})"sv),
            E(InvalidArgument));
  EXPECT_EQ(Run(R"({"model_path":"/models/missing"})"sv), E(LoadFailed));
  EXPECT_EQ(Run(R"({"model_path":"/work/w.bin"})"sv), E(NotReadOnly));
  Fake->OpenError = ErrNo::LoadFailed;
  EXPECT_EQ(Run(R"({"model_path":"/models/m.bin"})"sv), E(LoadFailed));
  Fake->OpenError = ErrNo::NotFound;
  EXPECT_EQ(Run(R"({"model_path":"/models/m.bin"})"sv), E(RuntimeError));
  Fake->OpenError.reset();
  Fake->OpenNull = true;
  EXPECT_EQ(Run(R"({"model_path":"/models/m.bin"})"sv), E(RuntimeError));
  EXPECT_EQ(G.u32(IdPtr), 0x7f7f7f7fU);
  EXPECT_EQ(Fake->LiveContexts, 0);
  EXPECT_EQ(Mod.getEnv().sessions().size(), 0U);
}

TEST_F(SDFuncTest, ARefusedPathNeverReachesTheBackend) {
  const auto Id = open();
  const auto Calls = static_cast<int>(Fake->Generates);
  EXPECT_EQ(
      generate(Id, R"({"prompt":"a","loras":[{"path":"/work/w.bin"}]})"sv),
      E(NotReadOnly));
  EXPECT_EQ(generate(Id, R"({"prompt":"a","loras":[{"path":"/models/no"}]})"sv),
            E(LoadFailed));
  EXPECT_EQ(Fake->Generates, Calls);
}

TEST_F(SDFuncTest, EveryStageOfGenerateFailsCleanly) {
  const auto Id = open();
  const auto Images = OwnedImage::liveCount();
  const auto CountPtr = G.put(std::string(4, '\0'));
  const auto Png = G.put(png(8, 8));
  const auto Table = G.blobs({{Png, pngSize(8, 8)}});
  const auto Run = [&](uint32_t Session, std::string_view Req, uint32_t Blobs,
                       uint32_t BlobCount, uint32_t Count) {
    const auto P = G.put(Req);
    return G.call(Mod, "generate",
                  {Session, P, static_cast<uint32_t>(Req.size()), Blobs,
                   BlobCount, Count});
  };
  EXPECT_EQ(Run(Id, "{"sv, 0, 0, CountPtr), E(InvalidEncoding));
  EXPECT_EQ(Run(Id, R"({"prompt":"a","steps":0})"sv, 0, 0, CountPtr),
            E(InvalidArgument));
  EXPECT_EQ(Run(Id + 1, R"({"prompt":"a"})"sv, 0, 0, CountPtr), E(NotFound));
  EXPECT_EQ(Run(Id, R"({"prompt":"a","control_image":{"blob":0}})"sv, Table, 1,
                CountPtr),
            E(InvalidArgument));
  EXPECT_EQ(Run(Id, R"({"prompt":"a"})"sv, Table, 9, CountPtr),
            E(InvalidArgument));
  EXPECT_EQ(G.call(Mod, "generate",
                   {Id, UINT32_C(0xffffffff), Limits::MaxTextBytes + 1,
                    UINT32_C(0), UINT32_C(0), CountPtr}),
            E(InvalidArgument));
  EXPECT_EQ(G.call(Mod, "generate",
                   {Id, G.memorySize() - 2, UINT32_C(10), UINT32_C(0),
                    UINT32_C(0), CountPtr}),
            E(MissingMemory));
  EXPECT_EQ(G.call(Mod, "generate",
                   {Id, G.memorySize() - 2, UINT32_C(10), UINT32_C(0),
                    UINT32_C(9), CountPtr}),
            E(MissingMemory));
  EXPECT_EQ(Run(Id, R"({"prompt":"a"})"sv, 0xfffffff0U, 9, CountPtr),
            E(InvalidArgument));
  EXPECT_EQ(Run(Id, R"({"prompt":"a"})"sv, 0xfffffff0U, 1, CountPtr),
            E(MissingMemory));
  const auto BadBlob = G.blobs({{0xfffffff0U, 64}});
  EXPECT_EQ(Run(Id, R"({"prompt":"a"})"sv, BadBlob, 1, CountPtr),
            E(MissingMemory));
  const auto Noise = G.put(std::string(64, '\x13'));
  const auto NoiseTable = G.blobs({{Noise, 64}});
  EXPECT_EQ(Run(Id, R"({"prompt":"a","init_image":{"blob":0}})"sv, NoiseTable,
                1, CountPtr),
            E(InvalidEncoding));
  Fake->OnGenerate = [](auto &, auto &, auto &) {
    return Unexpect(BackendFailure{ErrNo::RuntimeError, true});
  };
  EXPECT_EQ(Run(Id, R"({"prompt":"a","init_image":{"blob":0}})"sv, Table, 1,
                CountPtr),
            E(RuntimeError));
  Fake->OnGenerate = nullptr;
  EXPECT_EQ(Run(Id, R"({"prompt":"a"})"sv, 0, 0, 0xfffffffeU),
            E(MissingMemory));
  EXPECT_EQ(Run(Id, R"({"prompt":"a"})"sv, 0, 0, G.memorySize() - 3),
            E(MissingMemory));
  EXPECT_EQ(Run(Id, "{"sv, 0, 0, G.memorySize() - 3), E(MissingMemory));
  EXPECT_EQ(OwnedImage::liveCount(), Images);
  EXPECT_EQ(Run(Id, R"({"prompt":"a"})"sv, 0, 0, G.memorySize() - 4),
            E(Success));
  EXPECT_EQ(G.u32(G.memorySize() - 4), 1U);
}

TEST_F(SDFuncTest, ASessionThatCannotTakeLorasRefusesThem) {
  Fake->Caps.Lora = false;
  const auto Id = open();
  const auto Calls = static_cast<int>(Fake->Generates);
  EXPECT_EQ(
      generate(Id, R"({"prompt":"a","loras":[{"path":"/models/m.bin"}]})"sv),
      E(InvalidArgument));
  EXPECT_EQ(Fake->Generates, Calls);
  EXPECT_EQ(generate(Id, R"({"prompt":"a"})"sv), E(Success));
}

TEST_F(SDFuncTest, AModuleInstanceHoldsAFewSessionsAtATime) {
  std::vector<uint32_t> Ids;
  for (uint32_t I = 0; I < Limits::MaxSessions; ++I) {
    Ids.push_back(open());
  }
  const auto Opens = static_cast<int>(Fake->Opens);
  const auto IdPtr = G.put(std::string(4, '\x7f'));
  EXPECT_EQ(openOn(Mod, kModelConfig, IdPtr), E(Busy));
  EXPECT_EQ(openOn(Mod, "{"sv, IdPtr), E(Busy));
  EXPECT_EQ(G.u32(IdPtr), 0x7f7f7f7fU);
  EXPECT_EQ(Fake->Opens, Opens);
  EXPECT_EQ(Fake->LiveContexts, static_cast<int>(Limits::MaxSessions));
  Host::SDModule Other{Fake, std::chrono::system_clock::now() +
                                 std::chrono::hours(1)};
  EXPECT_EQ(openOn(Other, kModelConfig, IdPtr), E(Success));
  EXPECT_EQ(G.call(Mod, "session_close", {Ids.back()}), E(Success));
  EXPECT_EQ(openOn(Mod, kModelConfig, IdPtr), E(Success));
  EXPECT_EQ(openOn(Mod, kModelConfig, IdPtr), E(Busy));
  EXPECT_EQ(Mod.getEnv().sessions().size(), Limits::MaxSessions);
}

TEST_F(SDFuncTest, AFailedOpenKeepsNoPlace) {
  const auto IdPtr = G.put(std::string(4, '\0'));
  Fake->OpenError = ErrNo::LoadFailed;
  for (uint32_t I = 0; I < Limits::MaxSessions; ++I) {
    EXPECT_EQ(openOn(Mod, "{"sv, IdPtr), E(InvalidEncoding));
    EXPECT_EQ(openOn(Mod, kModelConfig, IdPtr), E(LoadFailed));
  }
  Fake->OpenError.reset();
  for (uint32_t I = 0; I < Limits::MaxSessions; ++I) {
    EXPECT_EQ(openOn(Mod, kModelConfig, IdPtr), E(Success));
  }
}

TEST_F(SDFuncTest, AClosedSessionKeepsItsPlaceUntilItsCallReturns) {
  std::vector<uint32_t> Ids;
  for (uint32_t I = 0; I < Limits::MaxSessions; ++I) {
    Ids.push_back(open());
  }
  std::promise<void> Inside;
  std::promise<void> Closed;
  auto WhenClosed = Closed.get_future();
  Fake->OnGenerate = [&](auto &Req, auto &, auto &) {
    Inside.set_value();
    WhenClosed.wait();
    return Expected<ImageBatch, BackendFailure>(
        solidImages(1, Req.Width, Req.Height));
  };
  const auto Request = R"({"prompt":"a","width":64,"height":64})"sv;
  const auto Text = G.put(Request);
  const auto CountPtr = G.put(std::string(4, '\0'));
  const auto IdPtr = G.put(std::string(4, '\0'));
  auto Generating = std::async(std::launch::async, [&] {
    return G.call(Mod, "generate",
                  {Ids[0], Text, static_cast<uint32_t>(Request.size()),
                   UINT32_C(0), UINT32_C(0), CountPtr});
  });
  EXPECT_EQ(Inside.get_future().wait_for(std::chrono::seconds(10)),
            std::future_status::ready);
  EXPECT_EQ(G.call(Mod, "session_close", {Ids[0]}), E(Success));
  EXPECT_EQ(openOn(Mod, kModelConfig, IdPtr), E(Busy));
  EXPECT_EQ(Fake->LiveContexts, static_cast<int>(Limits::MaxSessions));
  Closed.set_value();
  EXPECT_EQ(Generating.get(), E(Success));
  EXPECT_EQ(openOn(Mod, kModelConfig, IdPtr), E(Success));
}

// A backend whose open() waits until every caller of the test is in open() or
// is back from session_open.
class GateBackend : public FakeBackend {
public:
  uint32_t Callers = 0;
  std::atomic<uint32_t> Inside{0}, Back{0};
  Expected<std::unique_ptr<BackendContext>, ErrNo>
  open(const SessionConfig &, const SessionFiles &) noexcept override {
    ++Inside;
    const auto GiveUp =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (Inside + Back < Callers &&
           std::chrono::steady_clock::now() < GiveUp) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    ++Opens;
    return std::unique_ptr<BackendContext>(
        std::make_unique<FakeContext>(*this));
  }
};

TEST_F(SDFuncTest, OpensAtTheSameTimeCreateNoMoreContextsThanPlaces) {
  constexpr uint32_t Callers = 3 * Limits::MaxSessions;
  const auto Gate = std::make_shared<GateBackend>();
  Gate->Callers = Callers;
  Host::SDModule Module{Gate, std::chrono::system_clock::now() +
                                  std::chrono::hours(1)};
  const auto Text = G.put(kModelConfig);
  std::vector<uint32_t> IdPtrs;
  for (uint32_t I = 0; I < Callers; ++I) {
    IdPtrs.push_back(G.put(std::string(4, '\0')));
  }
  std::vector<std::future<uint32_t>> Calls;
  for (const auto IdPtr : IdPtrs) {
    Calls.push_back(std::async(std::launch::async, [&, IdPtr] {
      const auto Code =
          G.call(Module, "session_open",
                 {Text, static_cast<uint32_t>(kModelConfig.size()), IdPtr});
      ++Gate->Back;
      return Code;
    }));
  }
  std::vector<uint32_t> Codes;
  for (auto &Call : Calls) {
    Codes.push_back(Call.get());
  }
  const auto Count = [&Codes](uint32_t Code) {
    return static_cast<uint32_t>(std::count(Codes.begin(), Codes.end(), Code));
  };
  EXPECT_EQ(Count(E(Success)), Limits::MaxSessions);
  EXPECT_EQ(Count(E(Busy)), Callers - Limits::MaxSessions);
  EXPECT_EQ(Gate->Opens, static_cast<int>(Limits::MaxSessions));
  EXPECT_EQ(Gate->LiveContexts, static_cast<int>(Limits::MaxSessions));
}

TEST_F(SDFuncTest, RejectedRequestsKeepThePreviousResults) {
  const auto Id = open();
  generateOnce(Id);
  const auto Calls = static_cast<int>(Fake->Generates);
  EXPECT_EQ(generate(Id, R"({"prompt":"a","steps":0})"sv), E(InvalidArgument));
  const auto Larger = R"({"prompt":"a","width":128,"height":128})"sv;
  EXPECT_EQ(G.call(Mod, "generate",
                   {Id, G.put(Larger), static_cast<uint32_t>(Larger.size()),
                    UINT32_C(0), UINT32_C(0), G.memorySize() - 3}),
            E(MissingMemory));
  EXPECT_EQ(Fake->Generates, Calls);
  const auto Info = G.put(std::string(16, '\0'));
  EXPECT_EQ(G.call(Mod, "result_info", {Id, UINT32_C(0), UINT32_C(0), Info}),
            E(Success));
  EXPECT_EQ(G.u32(Info), 64U);
}

TEST_F(SDFuncTest, InputsReachTheBackendWithTheRightChannels) {
  Fake->Caps.ControlNet = true;
  const auto Id = open();
  uint32_t InitC = 0, MaskC = 0, ControlC = 0;
  Fake->OnGenerate = [&](auto &Req, auto &, auto &In) {
    InitC = In.Init->channels();
    MaskC = In.Mask->channels();
    ControlC = In.Control->channels();
    return Expected<ImageBatch, BackendFailure>(
        solidImages(1, Req.Width, Req.Height));
  };
  const auto Png = G.put(png(8, 8));
  const auto Table = G.blobs({{Png, pngSize(8, 8)}});
  ASSERT_EQ(generate(Id,
                     R"({"prompt":"a","init_image":{"blob":0},)"
                     R"("mask_image":{"blob":0},"control_image":{"blob":0}})"sv,
                     Table, 1),
            E(Success));
  EXPECT_EQ(InitC, 3U);
  EXPECT_EQ(MaskC, 1U);
  EXPECT_EQ(ControlC, 3U);
}

TEST_F(SDFuncTest, CallsThatWaitForTheirSessionHoldNoDecodedImage) {
  const auto Id = open();
  std::promise<void> Inside, Release;
  std::atomic<int> Generations{0};
  Fake->OnGenerate = [&](auto &Req, auto &, auto &) {
    if (Generations++ == 0) {
      Inside.set_value();
      Release.get_future().wait();
    }
    return Expected<ImageBatch, BackendFailure>(
        solidImages(1, Req.Width, Req.Height));
  };
  const auto Plain = R"({"prompt":"a","width":64,"height":64})"sv;
  const auto WithImage =
      R"({"prompt":"a","width":64,"height":64,"init_image":{"blob":0}})"sv;
  const auto PlainText = G.put(Plain);
  const auto ImageText = G.put(WithImage);
  const auto Table = G.blobs({{G.put(png(8, 8)), pngSize(8, 8)}});
  constexpr size_t Waiting = 4;
  std::vector<uint32_t> Counts;
  for (size_t I = 0; I <= Waiting; ++I) {
    Counts.push_back(G.put(std::string(4, '\0')));
  }
  auto First = std::async(std::launch::async, [&] {
    return G.call(Mod, "generate",
                  {Id, PlainText, static_cast<uint32_t>(Plain.size()),
                   UINT32_C(0), UINT32_C(0), Counts[Waiting]});
  });
  ASSERT_EQ(Inside.get_future().wait_for(std::chrono::seconds(10)),
            std::future_status::ready);
  const auto Live = OwnedImage::liveCount();
  std::vector<std::future<uint32_t>> Calls;
  for (size_t I = 0; I < Waiting; ++I) {
    Calls.push_back(std::async(std::launch::async, [&, I] {
      return G.call(Mod, "generate",
                    {Id, ImageText, static_cast<uint32_t>(WithImage.size()),
                     Table, UINT32_C(1), Counts[I]});
    }));
  }
  // Time for the calls to reach the session and wait there.
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_EQ(OwnedImage::liveCount(), Live);
  Release.set_value();
  EXPECT_EQ(First.get(), E(Success));
  for (auto &Call : Calls) {
    EXPECT_EQ(Call.get(), E(Success));
  }
}

TEST_F(SDFuncTest, TheMockHasTheSameFunctionTypes) {
  WasmEdge::Host::WasmEdgeStableDiffusionModuleMock Mock;
  for (const auto Name : {"abi_version"sv, "session_open"sv, "session_close"sv,
                          "generate"sv, "result_info"sv, "result_read"sv}) {
    auto *Real = Mod.findFuncExports(Name);
    auto *Stand = Mock.findFuncExports(Name);
    ASSERT_NE(Real, nullptr) << Name;
    ASSERT_NE(Stand, nullptr) << Name;
    EXPECT_EQ(Real->getFuncType(), Stand->getFuncType()) << Name;
  }
  EXPECT_EQ(Mod.getFuncExportNum(), 6U);
  EXPECT_EQ(Mock.getFuncExportNum(), 6U);
}

TEST_F(SDFuncTest, ResultBuffersAtTheEdges) {
  const auto Id = open();
  generateOnce(Id);
  const uint32_t Size = 64U * 64U * 3U;
  const auto Written = G.put(std::string(4, '\0'));
  EXPECT_EQ(
      G.call(Mod, "result_read",
             {Id, UINT32_C(0), UINT32_C(1), UINT32_C(0), UINT32_C(0), Written}),
      E(BufferTooSmall));
  EXPECT_EQ(G.u32(Written), Size);
  const auto Buf = G.put(std::string(Size, '\x55'));
  EXPECT_EQ(G.call(Mod, "result_read",
                   {Id, UINT32_C(0), UINT32_C(1), Buf, Size - 1, Written}),
            E(BufferTooSmall));
  EXPECT_EQ(G.byte(Buf), 0x55U);
  EXPECT_EQ(
      G.call(Mod, "result_read",
             {Id, UINT32_C(0), UINT32_C(1), Buf, Size, G.memorySize() - 1}),
      E(MissingMemory));
  EXPECT_EQ(G.byte(Buf), 0x55U);
  EXPECT_EQ(G.call(Mod, "result_read",
                   {Id, UINT32_C(0), UINT32_C(1), UINT32_C(0xfffffff0), Size,
                    Written}),
            E(MissingMemory));
  const auto Untouched = G.put(std::string(4, '\x7f'));
  EXPECT_EQ(G.call(Mod, "result_read",
                   {Id, UINT32_C(0), UINT32_C(1), Buf, UINT32_C(0xffffffff),
                    Untouched}),
            E(MissingMemory));
  EXPECT_EQ(G.byte(Buf), 0x55U);
  EXPECT_EQ(G.byte(Buf + Size - 1), 0x55U);
  EXPECT_EQ(G.u32(Untouched), 0x7f7f7f7fU);
  EXPECT_EQ(G.call(Mod, "result_read",
                   {Id, UINT32_C(0), UINT32_C(1), G.memorySize() - 4,
                    UINT32_C(100), Untouched}),
            E(MissingMemory));
  EXPECT_EQ(G.u32(Untouched), 0x7f7f7f7fU);
  EXPECT_EQ(G.call(Mod, "result_info",
                   {Id, UINT32_C(0), UINT32_C(1), G.memorySize() - 15}),
            E(MissingMemory));
  EXPECT_EQ(G.call(Mod, "result_info", {Id, UINT32_C(0), UINT32_C(7), Written}),
            E(InvalidArgument));
  EXPECT_EQ(G.call(Mod, "result_read",
                   {Id, UINT32_C(3), UINT32_C(1), Buf, Size, Written}),
            E(NotFound));
  const uint32_t Outside = 0xfffffff0U;
  EXPECT_EQ(G.call(Mod, "result_read",
                   {Id + 1, UINT32_C(0), UINT32_C(1), Outside, Size, Outside}),
            E(NotFound));
  EXPECT_EQ(G.call(Mod, "result_read",
                   {Id, UINT32_C(0), UINT32_C(7), Outside, Size, Outside}),
            E(InvalidArgument));
  EXPECT_EQ(G.call(Mod, "result_read",
                   {Id, UINT32_C(3), UINT32_C(1), Outside, Size, Outside}),
            E(NotFound));
  const auto AtTheEnd = G.memorySize() - Size;
  EXPECT_EQ(G.call(Mod, "result_read",
                   {Id, UINT32_C(0), UINT32_C(1), AtTheEnd, Size, Written}),
            E(Success));
  EXPECT_EQ(G.byte(AtTheEnd), 0x80U);
  EXPECT_EQ(G.byte(G.memorySize() - 1), 0x80U);
  const auto LastFour = G.memorySize() - 4;
  EXPECT_EQ(G.call(Mod, "result_read",
                   {Id, UINT32_C(0), UINT32_C(1), Buf, Size, LastFour}),
            E(Success));
  EXPECT_EQ(G.u32(LastFour), Size);
}

TEST_F(SDFuncTest, AWrittenPtrInTheResultBufferIsRefused) {
  const auto Id = open();
  generateOnce(Id);
  const uint32_t Size = 64U * 64U * 3U;
  // Four bytes, the buffer, and four bytes more, in one block.
  const auto Before = G.put(std::string(Size + 8, '\x55'));
  const auto Buf = Before + 4;
  const auto After = Buf + Size;
  const auto Unchanged = [&] {
    for (uint32_t At = Before; At < After + 4; ++At) {
      if (G.byte(At) != 0x55U) {
        return false;
      }
    }
    return true;
  };
  for (const uint32_t BufLen : {Size, Size - 1}) {
    for (const uint32_t WrittenPtr :
         {Buf - 3, Buf, Buf + 100, Buf + BufLen - 1}) {
      EXPECT_EQ(G.call(Mod, "result_read",
                       {Id, UINT32_C(0), UINT32_C(1), Buf, BufLen, WrittenPtr}),
                E(InvalidArgument))
          << BufLen << " " << WrittenPtr - Buf;
      EXPECT_TRUE(Unchanged()) << BufLen << " " << WrittenPtr - Buf;
    }
  }
  // An empty buffer overlaps nothing.
  EXPECT_EQ(G.call(Mod, "result_read",
                   {Id, UINT32_C(0), UINT32_C(1), Buf, UINT32_C(0), Buf}),
            E(BufferTooSmall));
  EXPECT_EQ(G.u32(Buf), Size);
  // The four bytes can be directly before or after the buffer.
  EXPECT_EQ(G.call(Mod, "result_read",
                   {Id, UINT32_C(0), UINT32_C(1), Buf, Size, Before}),
            E(Success));
  EXPECT_EQ(G.u32(Before), Size);
  EXPECT_EQ(G.call(Mod, "result_read",
                   {Id, UINT32_C(0), UINT32_C(1), Buf, Size, After}),
            E(Success));
  EXPECT_EQ(G.u32(After), Size);
  EXPECT_EQ(G.byte(Buf), 0x80U);
  EXPECT_EQ(G.byte(Buf + Size - 1), 0x80U);
}

TEST_F(SDFuncTest, TwoModulesOnOneBackendKeepTheirSessionsApart) {
  Host::SDModule Other{Fake, std::chrono::system_clock::now() +
                                 std::chrono::hours(1)};
  const auto Mine = open();
  const auto Req = R"({"prompt":"a"})"sv;
  const auto Text = G.put(Req);
  const auto Info = G.put(std::string(16, '\0'));
  const auto Generate = [&](Host::SDModule &Module, uint32_t Id) {
    return G.call(Module, "generate",
                  {Id, Text, static_cast<uint32_t>(Req.size()), UINT32_C(0),
                   UINT32_C(0), Info});
  };
  EXPECT_EQ(Generate(Other, Mine), E(NotFound));
  EXPECT_EQ(
      G.call(Other, "result_info", {Mine, UINT32_C(0), UINT32_C(1), Info}),
      E(NotFound));
  EXPECT_EQ(G.call(Other, "result_read",
                   {Mine, UINT32_C(0), UINT32_C(1), Info, UINT32_C(16), Info}),
            E(NotFound));
  EXPECT_EQ(G.call(Other, "session_close", {Mine}), E(NotFound));
  const auto IdPtr = G.put(std::string(4, '\0'));
  ASSERT_EQ(openOn(Other, kModelConfig, IdPtr), E(Success));
  const auto Theirs = G.u32(IdPtr);
  EXPECT_EQ(Fake->LiveContexts, 2);
  EXPECT_EQ(G.call(Mod, "session_close", {Mine}), E(Success));
  EXPECT_EQ(Generate(Mod, Mine), E(NotFound));
  EXPECT_EQ(Generate(Other, Theirs), E(Success));
  EXPECT_EQ(G.call(Other, "session_close", {Theirs}), E(Success));
  EXPECT_EQ(Fake->LiveContexts, 0);
}

TEST_F(SDFuncTest, UnusualBlobs) {
  const auto Id = open();
  const auto CountPtr = G.put(std::string(4, '\0'));
  const auto Empty = G.blobs({{UINT32_C(0), UINT32_C(0)}});
  const auto Plain = G.put(R"({"prompt":"a"})"sv);
  EXPECT_EQ(G.call(Mod, "generate",
                   {Id, Plain, UINT32_C(14), Empty, UINT32_C(1), CountPtr}),
            E(Success));
  const auto Text = R"({"prompt":"a","init_image":{"blob":0}})"sv;
  const auto Uses = G.put(Text);
  EXPECT_EQ(G.call(Mod, "generate",
                   {Id, Uses, static_cast<uint32_t>(Text.size()), Empty,
                    UINT32_C(1), CountPtr}),
            E(InvalidEncoding));
}

TEST_F(SDFuncTest, StaleAndUnknownSessionIds) {
  const auto Info = G.put(std::string(16, '\0'));
  const auto Req = G.put(R"({"prompt":"a"})"sv);
  const auto Every = [&](uint32_t Id) {
    EXPECT_EQ(G.call(Mod, "generate",
                     {Id, Req, UINT32_C(14), UINT32_C(0), UINT32_C(0), Info}),
              E(NotFound));
    EXPECT_EQ(G.call(Mod, "result_info", {Id, UINT32_C(0), UINT32_C(0), Info}),
              E(NotFound));
    EXPECT_EQ(G.call(Mod, "result_read",
                     {Id, UINT32_C(0), UINT32_C(0), Info, UINT32_C(16), Info}),
              E(NotFound));
    EXPECT_EQ(G.call(Mod, "session_close", {Id}), E(NotFound));
  };
  Every(0);
  Every(UINT32_MAX);
  const auto Id = open();
  EXPECT_EQ(G.call(Mod, "session_close", {Id}), E(Success));
  Every(Id);
  EXPECT_NE(open(), Id);
}

TEST_F(SDFuncTest, AModuleWithoutABackendRefusesToOpen) {
  Host::SDModule Bare{nullptr};
  const auto IdPtr = G.put(std::string(4, '\x7f'));
  EXPECT_EQ(openOn(Bare, kModelConfig, IdPtr), E(RuntimeError));
  EXPECT_EQ(G.u32(IdPtr), 0x7f7f7f7fU);
  EXPECT_EQ(Bare.getEnv().sessions().size(), 0U);
  EXPECT_EQ(G.call(Bare, "abi_version", {}), kAbiVersion);
  const auto Info = G.put(std::string(16, '\0'));
  const auto Req = R"({"prompt":"a"})"sv;
  const auto Text = G.put(Req);
  EXPECT_EQ(G.call(Bare, "generate",
                   {UINT32_C(0), Text, static_cast<uint32_t>(Req.size()),
                    UINT32_C(0), UINT32_C(0), Info}),
            E(NotFound));
  EXPECT_EQ(G.call(Bare, "result_info",
                   {UINT32_C(0), UINT32_C(0), UINT32_C(0), Info}),
            E(NotFound));
  EXPECT_EQ(
      G.call(Bare, "result_read",
             {UINT32_C(0), UINT32_C(0), UINT32_C(0), Info, UINT32_C(16), Info}),
      E(NotFound));
  EXPECT_EQ(G.call(Bare, "session_close", {UINT32_C(0)}), E(NotFound));
}

TEST_F(SDFuncTest, AFileChangedAfterTheModuleWasCreatedIsRefused) {
  Host::SDModule Fresh{Fake};
  std::ofstream(Root / "models" / "m.bin", std::ios::binary | std::ios::app)
      << 'X';
  const auto Opens = static_cast<int>(Fake->Opens);
  EXPECT_EQ(openOn(Fresh, kModelConfig, G.put(std::string(4, '\0'))),
            E(NotReadOnly));
  EXPECT_EQ(Fake->Opens, Opens);
  EXPECT_EQ(openOn(Mod, kModelConfig, G.put(std::string(4, '\0'))), E(Success));
}

TEST_F(SDFuncTest, ALoraChangedAfterTheModuleWasCreatedIsRefused) {
  Host::SDModule Fresh{Fake, settledAfter(Root / "models" / "m.bin")};
  const auto IdPtr = G.put(std::string(4, '\0'));
  ASSERT_EQ(openOn(Fresh, kModelConfig, IdPtr), E(Success));
  const auto Id = G.u32(IdPtr);
  std::ofstream(Root / "models" / "l.bin", std::ios::binary) << "LORA";
  const auto Req = R"({"prompt":"a","loras":[{"path":"/models/l.bin"}]})"sv;
  const auto Text = G.put(Req);
  const auto CountPtr = G.put(std::string(4, '\0'));
  const auto Generates = static_cast<int>(Fake->Generates);
  EXPECT_EQ(G.call(Fresh, "generate",
                   {Id, Text, static_cast<uint32_t>(Req.size()), UINT32_C(0),
                    UINT32_C(0), CountPtr}),
            E(NotReadOnly));
  EXPECT_EQ(Fake->Generates, Generates);
  EXPECT_EQ(generate(open(), Req), E(Success));
}

TEST_F(SDFuncTest, ResultInfoGivesEachFieldOfANonSquareResult) {
  const auto Id = open();
  ASSERT_EQ(generate(Id, R"({"prompt":"a","width":128,"height":64})"sv),
            E(Success));
  const auto Info = G.put(std::string(16, '\0'));
  ASSERT_EQ(G.call(Mod, "result_info", {Id, UINT32_C(0), UINT32_C(1), Info}),
            E(Success));
  EXPECT_EQ(G.u32(Info), 128U);
  EXPECT_EQ(G.u32(Info + 4), 64U);
  EXPECT_EQ(G.u32(Info + 8), 3U);
  EXPECT_EQ(G.u32(Info + 12), 128U * 64U * 3U);
}

TEST_F(SDFuncTest, ASessionClosedDuringGenerateLivesUntilGenerateReturns) {
  const auto Id = open();
  std::promise<void> Inside;
  std::promise<void> Closed;
  auto WhenClosed = Closed.get_future();
  Fake->OnGenerate = [&](auto &Req, auto &, auto &) {
    Inside.set_value();
    WhenClosed.wait();
    return Expected<ImageBatch, BackendFailure>(
        solidImages(1, Req.Width, Req.Height));
  };
  const auto Request = R"({"prompt":"a","width":64,"height":64})"sv;
  const auto Text = G.put(Request);
  const auto CountPtr = G.put(std::string(4, '\0'));
  auto Generating = std::async(std::launch::async, [&] {
    return G.call(Mod, "generate",
                  {Id, Text, static_cast<uint32_t>(Request.size()), UINT32_C(0),
                   UINT32_C(0), CountPtr});
  });
  EXPECT_EQ(Inside.get_future().wait_for(std::chrono::seconds(10)),
            std::future_status::ready);
  EXPECT_EQ(G.call(Mod, "session_close", {Id}), E(Success));
  EXPECT_EQ(Fake->LiveContexts, 1);
  Closed.set_value();
  EXPECT_EQ(Generating.get(), E(Success));
  EXPECT_EQ(G.u32(CountPtr), 1U);
  EXPECT_EQ(Fake->LiveContexts, 0);
}

TEST_F(SDFuncTest, EachFailureLogsOneErrorLineThatNamesTheFunction) {
  const auto Id = open();
  generateOnce(Id);
  const auto Written = G.put(std::string(4, '\0'));
  const auto Config = R"({"model_path":"/models/SECRET","n_threads":0})"sv;
  const auto Req = R"({"prompt":"SECRET","steps":0})"sv;
  const auto ConfigPtr = G.put(Config);
  const auto ReqPtr = G.put(Req);
  std::ostringstream Out;
  auto Logger = std::make_shared<spdlog::logger>(
      "sd-func-lines", std::make_shared<spdlog::sinks::ostream_sink_mt>(Out));
  Logger->set_pattern("%l|%v");
  Logger->set_level(spdlog::level::trace);
  const auto Previous = spdlog::default_logger();
  spdlog::set_default_logger(Logger);
  std::vector<std::string> All;
  // Moves the lines logged since the last call into All. Returns the lines of
  // host function Function at level Level.
  const auto Taken = [&](std::string_view Level, std::string_view Function) {
    std::vector<std::string> Own;
    std::istringstream In(Out.str());
    for (std::string Line; std::getline(In, Line);) {
      if (Line.rfind(std::string(Level) + "|[WasmEdge-StableDiffusion] " +
                         std::string(Function) + " ",
                     0) == 0) {
        Own.push_back(Line);
      }
      All.push_back(Line);
    }
    Out.str("");
    return Own;
  };
  const auto One = [](std::string Line) {
    return std::vector<std::string>{std::move(Line)};
  };
  EXPECT_EQ(G.call(Mod, "session_open",
                   {ConfigPtr, static_cast<uint32_t>(Config.size()), Written}),
            E(InvalidArgument));
  EXPECT_EQ(Taken("error", "session_open"),
            One("error|[WasmEdge-StableDiffusion] session_open failed with "
                "InvalidArgument"));
  EXPECT_EQ(G.call(Mod, "session_close", {Id + 1}), E(NotFound));
  EXPECT_EQ(Taken("error", "session_close"),
            One("error|[WasmEdge-StableDiffusion] session_close failed with "
                "NotFound: no open session has this id"));
  EXPECT_EQ(G.call(Mod, "generate",
                   {Id, ReqPtr, static_cast<uint32_t>(Req.size()), UINT32_C(0),
                    UINT32_C(0), Written}),
            E(InvalidArgument));
  EXPECT_EQ(Taken("error", "generate"),
            One("error|[WasmEdge-StableDiffusion] generate failed with "
                "InvalidArgument"));
  EXPECT_EQ(G.call(Mod, "result_info", {Id, UINT32_C(0), UINT32_C(7), Written}),
            E(InvalidArgument));
  EXPECT_EQ(Taken("error", "result_info"),
            One("error|[WasmEdge-StableDiffusion] result_info failed with "
                "InvalidArgument"));
  EXPECT_EQ(G.call(Mod, "result_read",
                   {Id, UINT32_C(0), UINT32_C(1), UINT32_C(0),
                    UINT32_C(64 * 64 * 3), G.memorySize() - 1}),
            E(MissingMemory));
  EXPECT_EQ(Taken("error", "result_read"),
            One("error|[WasmEdge-StableDiffusion] result_read failed with "
                "MissingMemory"));
  const auto Before = All.size();
  EXPECT_EQ(
      G.call(Mod, "result_read",
             {Id, UINT32_C(0), UINT32_C(1), UINT32_C(0), UINT32_C(0), Written}),
      E(BufferTooSmall));
  EXPECT_EQ(Taken("debug", "result_read"),
            One("debug|[WasmEdge-StableDiffusion] result_read failed with "
                "BufferTooSmall"));
  EXPECT_EQ(All.size(), Before + 1);
  spdlog::set_default_logger(Previous);
  spdlog::drop("sd-func-lines");
  for (const auto &Line : All) {
    EXPECT_EQ(Line.find("SECRET"), std::string::npos) << Line;
  }
}

TEST_F(SDFuncTest, ACallerWithoutAMemoryGetsMissingMemory) {
  const auto Id = open();
  generateOnce(Id);
  const auto Opens = static_cast<int>(Fake->Opens);
  const auto Generates = static_cast<int>(Fake->Generates);
  EXPECT_EQ(G.callWithoutMemory(Mod, "session_open",
                                {UINT32_C(0), UINT32_C(30), UINT32_C(0)}),
            E(MissingMemory));
  EXPECT_EQ(G.callWithoutMemory(Mod, "generate",
                                {Id, UINT32_C(0), UINT32_C(14), UINT32_C(0),
                                 UINT32_C(0), UINT32_C(0)}),
            E(MissingMemory));
  EXPECT_EQ(G.callWithoutMemory(Mod, "result_info",
                                {Id, UINT32_C(0), UINT32_C(1), UINT32_C(0)}),
            E(MissingMemory));
  EXPECT_EQ(G.callWithoutMemory(Mod, "result_read",
                                {Id, UINT32_C(0), UINT32_C(1), UINT32_C(0),
                                 UINT32_C(64 * 64 * 3), UINT32_C(0)}),
            E(MissingMemory));
  EXPECT_EQ(G.callWithoutMemory(Mod, "result_read",
                                {Id, UINT32_C(0), UINT32_C(1), UINT32_C(0),
                                 UINT32_C(0), UINT32_C(0)}),
            E(MissingMemory));
  EXPECT_EQ(Fake->Opens, Opens);
  EXPECT_EQ(Fake->Generates, Generates);
  EXPECT_EQ(G.callWithoutMemory(Mod, "session_close", {Id}), E(Success));
}

} // namespace
