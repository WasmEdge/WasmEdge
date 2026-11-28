// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "sd_abi.h"
#include "test_support.h"

#include "common/spdlog.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <future>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#define E(x) static_cast<uint32_t>(ErrNo::x)

namespace {

using namespace std::literals;
using namespace WasmEdge::Host::StableDiffusion;

TEST_F(SDContractTest, PromptsThatUsedToEndTheHostReturnAnImage) {
  const auto Id = session();
  for (const std::string &Prompt :
       {std::string("(a:.)"), std::string("<lora:x:y>"), repeat("cat ", 375),
        std::string(1500, ','), std::string(1500, '!')}) {
    EXPECT_EQ(generate(Id, small("", Prompt)), E(Success))
        << Prompt.substr(0, 16);
  }
  EXPECT_EQ(generate(Id, small()), E(Success));
}

TEST_F(SDContractTest, PromptTextCannotNameFiles) {
  const auto Id = session();
  const std::string Model =
      std::filesystem::canonical(modelDir() / "sd-v1-5-fp16.safetensors")
          .string();
  for (const std::string &Text :
       {std::string("<embedding:/etc/hosts>"),
        std::string("<image:/etc/hosts>"), "<embedding:" + Model + ">",
        "a cat <embedding:" + Model + "> on a mat",
        "\\<embedding:" + Model + ">"}) {
    EXPECT_EQ(generate(Id, small("", Text)), E(InvalidArgument)) << Text;
    EXPECT_EQ(generate(Id, small(",\"negative_prompt\":" + jsonString(Text))),
              E(InvalidArgument))
        << Text;
  }
  EXPECT_EQ(generate(Id, small()), E(Success));
}

TEST_F(SDContractTest, AConfiguredEmbeddingChangesTheImage) {
  writeSafetensors("e1.safetensors", {{"emb_params", "F32", {2, 768}, 0.5F}});
  const auto Plain = session();
  ASSERT_EQ(generate(Plain, small("", "a photo of e1")), E(Success));
  const auto Without = hash(Plain, 0);
  ASSERT_EQ(generate(Plain, small()), E(Success));
  const auto Cat = hash(Plain, 0);
  // Only one session holds the model at a time: a CI runner has little memory.
  ASSERT_EQ(close(Plain), E(Success));
  const auto Id =
      session(R"(,"embeddings":[{"name":"e1","path":"/aux/e1.safetensors"}])");
  ASSERT_EQ(generate(Id, small("", "a photo of e1")), E(Success));
  EXPECT_NE(hash(Id, 0), Without);
  ASSERT_EQ(generate(Id, small()), E(Success));
  EXPECT_EQ(hash(Id, 0), Cat);
}

TEST_F(SDContractTest, FilesThatAreNotModelsFailToOpen) {
  writeBytes("random.safetensors", std::string(4096, '\x5a'));
  writeBytes("text.safetensors", "this is not a model\n");
  writeTruncatedCopyOfTheModel("truncated.safetensors", 1U << 20);
  for (const auto Path :
       {"/aux/missing.safetensors"sv, "/aux/random.safetensors"sv,
        "/aux/text.safetensors"sv, "/aux/truncated.safetensors"sv}) {
    EXPECT_EQ(openCode("{\"model_path\":" + jsonString(std::string(Path)) +
                       ",\"device\":\"cpu\"}"),
              E(LoadFailed))
        << Path;
  }
  EXPECT_EQ(generate(session(), small()), E(Success));
}

TEST_F(SDContractTest, ADamagedVaeFailsGenerationAndKeepsTheSession) {
  writeBytes("vae.safetensors", std::string(4096, '\x5a'));
  const auto Id = session(R"(,"vae_path":"/aux/vae.safetensors")");
  EXPECT_EQ(generate(Id, small()), E(LoadFailed));
  EXPECT_EQ(generate(Id, small()), E(LoadFailed));
}

TEST_F(SDContractTest, ALoraIsAppliedForOneCallOnly) {
  writeSafetensors(
      "ok.safetensors",
      {{std::string(kLoraBase) + ".lora_down.weight", "F32", {4, 320}, 0.05F},
       {std::string(kLoraBase) + ".lora_up.weight", "F32", {320, 4}, 0.05F}});
  const auto Id = session();
  ASSERT_EQ(generate(Id, small()), E(Success));
  const auto Base = hash(Id, 0);
  ASSERT_EQ(generate(Id, small(R"(,"loras":[{"path":"/aux/ok.safetensors",)"
                               R"("multiplier":1.0}])")),
            E(Success));
  EXPECT_NE(hash(Id, 0), Base);
  ASSERT_EQ(generate(Id, small()), E(Success));
  EXPECT_EQ(hash(Id, 0), Base);
}

TEST_F(SDContractTest, ALoraThatDoesNotFitTheModelIsNotFatal) {
  writeSafetensors(
      "misfit.safetensors",
      {{std::string(kLoraBase) + ".lora_down.weight", "F32", {4, 300}, 0.05F},
       {std::string(kLoraBase) + ".lora_up.weight", "F32", {300, 4}, 0.05F}});
  const auto Id = session();
  const auto Code =
      generate(Id, small(R"(,"loras":[{"path":"/aux/misfit.safetensors"}])"));
  EXPECT_TRUE(Code == E(Success) || Code == E(LoadFailed) ||
              Code == E(RuntimeError))
      << Code;
  EXPECT_EQ(generate(Id, small()), E(Success));
}

TEST_F(SDContractTest, ASessionThatAskedForQuantizedWeightsRefusesLoras) {
  writeSafetensors(
      "fits.safetensors",
      {{std::string(kLoraBase) + ".lora_down.weight", "F32", {4, 320}, 0.05F},
       {std::string(kLoraBase) + ".lora_up.weight", "F32", {320, 4}, 0.05F}});
  const auto Id = session(R"(,"weight_type":"q8_0")");
  EXPECT_EQ(
      generate(Id, small(R"(,"loras":[{"path":"/aux/fits.safetensors"}])")),
      E(InvalidArgument));
  EXPECT_EQ(generate(Id, small()), E(Success));
}

TEST_F(SDContractTest, AModuleInstanceHoldsAFewSessionsAtATime) {
  std::vector<uint32_t> Ids;
  for (uint32_t I = 0; I < Limits::MaxSessions; ++I) {
    Ids.push_back(session());
  }
  const auto Config =
      R"({"model_path":"/models/sd-v1-5-fp16.safetensors","device":"cpu"})"sv;
  EXPECT_EQ(openCode(Config), E(Busy));
  EXPECT_EQ(generate(Ids.front(), small()), E(Success));
  // Close the session that holds the model: a CI runner has little memory.
  EXPECT_EQ(close(Ids.front()), E(Success));
  uint32_t Id = UINT32_MAX;
  EXPECT_EQ(openCode(Config, &Id), E(Success));
  EXPECT_EQ(generate(Id, small()), E(Success));
}

TEST_F(SDContractTest, CallsThatOpenAtOnceDoNotPassTheLimit) {
  const auto Config =
      R"({"model_path":"/models/sd-v1-5-fp16.safetensors","device":"cpu"})"sv;
  std::vector<std::future<uint32_t>> Runs;
  for (int I = 0; I < 12; ++I) {
    Runs.push_back(std::async(std::launch::async, [&] {
      Guest Mine = guestLike(G);
      const auto Text = Mine.put(Config);
      return Mine.call(*Mod, "session_open",
                       {Text, static_cast<uint32_t>(Config.size()),
                        Mine.put(std::string(4, '\0'))});
    }));
  }
  uint32_t Opened = 0;
  for (auto &R : Runs) {
    const auto Code = R.get();
    EXPECT_TRUE(Code == E(Success) || Code == E(Busy)) << Code;
    Opened += Code == E(Success) ? 1 : 0;
  }
  EXPECT_EQ(Opened, Limits::MaxSessions);
  EXPECT_EQ(openCode(Config), E(Busy));
}

TEST_F(SDContractTest, TheRowsThatMentionedEmbeddingsStandForAreLimited) {
  writeSafetensors("rows.safetensors",
                   {{"emb_params", "F32", {600, 768}, 0.01F}});
  const auto Id = session(
      R"(,"embeddings":[{"name":"rows","path":"/aux/rows.safetensors"}])");
  ASSERT_EQ(generate(Id, small()), E(Success));
  const auto Plain = hash(Id, 0);
  ASSERT_EQ(generate(Id, small("", "a cat, rows")), E(Success));
  EXPECT_NE(hash(Id, 0), Plain);
  EXPECT_EQ(generate(Id, small("", "a cat, rows, ROWS")), E(InvalidArgument));
  EXPECT_EQ(generate(Id, small(R"(,"negative_prompt":"rows")", "a cat, rows")),
            E(InvalidArgument));
  ASSERT_EQ(generate(Id, small()), E(Success));
  EXPECT_EQ(hash(Id, 0), Plain);
}

TEST_F(SDContractTest, LoraFilesTheLibraryAssertsOnAreRefused) {
  const std::string Down = std::string(kLoraBase) + ".lora_down.weight";
  const std::string Up = std::string(kLoraBase) + ".lora_up.weight";
  const std::string Alpha = std::string(kLoraBase) + ".alpha";
  writeSafetensors("ranks.safetensors", {{Down, "F32", {2, 320}, 0.1F},
                                         {Up, "F32", {320, 3}, 0.1F}});
  writeSafetensors("zero-down.safetensors", {{Down, "F32", {0, 320}, 0.1F},
                                             {Up, "F32", {320, 2}, 0.1F}});
  writeSafetensors("zero-up.safetensors",
                   {{Down, "F32", {2, 320}, 0.1F}, {Up, "F32", {0, 2}, 0.1F}});
  writeSafetensors("flat.safetensors",
                   {{Down, "F32", {640}, 0.1F}, {Up, "F32", {640}, 0.1F}});
  writeSafetensors("alpha3.safetensors", {{Down, "F32", {2, 320}, 0.1F},
                                          {Up, "F32", {320, 2}, 0.1F},
                                          {Alpha, "F32", {3}, 1.0F}});
  writeSafetensors("ints.safetensors", {{Down, "I64", {2, 320}, 0.0F},
                                        {Up, "I64", {320, 2}, 0.0F}});
  writeBytes("damaged.safetensors", std::string(4096, '\x5a'));
  const auto Id = session();
  ASSERT_EQ(generate(Id, small()), E(Success));
  const auto Base = hash(Id, 0);
  for (const auto Name : {"ranks"sv, "zero-down"sv, "zero-up"sv, "flat"sv,
                          "alpha3"sv, "ints"sv, "damaged"sv}) {
    EXPECT_EQ(generate(Id, small(",\"loras\":[{\"path\":\"/aux/" +
                                 std::string(Name) + ".safetensors\"}]")),
              E(LoadFailed))
        << Name;
    ASSERT_EQ(generate(Id, small()), E(Success)) << Name;
    EXPECT_EQ(hash(Id, 0), Base) << Name;
  }
}

TEST_F(SDContractTest, AnOversizedEmbeddingIsRefusedAtOpen) {
  writeSparse(
      "big-embedding.safetensors", (UINT64_C(16) << 20) + 1,
      R"({"emb_params":{"dtype":"F32","shape":[2,768],"data_offsets":[0,6144]}})");
  EXPECT_EQ(openCode(R"({"model_path":"/models/sd-v1-5-fp16.safetensors",)"
                     R"("device":"cpu","embeddings":[{"name":"big",)"
                     R"("path":"/aux/big-embedding.safetensors"}]})"),
            E(LoadFailed));
}

TEST_F(SDContractTest, AnEmbeddingWhoseTensorsShareTheirDataIsRefusedAtOpen) {
  std::string Header = "{";
  for (const auto Name : {"t0"sv, "t1"sv, "t2"sv, "t3"sv}) {
    Header += (Header.size() > 1 ? ",\"" : "\"") + std::string(Name) +
              R"(":{"dtype":"F16","shape":[10900,768],)"
              R"("data_offsets":[0,16742400]})";
  }
  Header += "}";
  Header.append((8 - Header.size() % 8) % 8, ' ');
  writeSparse("shared-embedding.safetensors",
              8 + Header.size() + UINT64_C(16742400), Header);
  EXPECT_EQ(openCode(R"({"model_path":"/models/sd-v1-5-fp16.safetensors",)"
                     R"("device":"cpu","embeddings":[{"name":"shared",)"
                     R"("path":"/aux/shared-embedding.safetensors"}]})"),
            E(LoadFailed));
}

TEST_F(SDContractTest, InputsOfAnotherSizeAndTilingAtTheSmallestSize) {
  const auto Id = session();
  ASSERT_EQ(generate(Id, small()), E(Success));
  const auto Plain = hash(Id, 0);
  const auto Png = png(96, 40);
  const auto Mask = png(17, 23);
  EXPECT_EQ(
      generate(Id,
               small(R"(,"init_image":{"blob":0},"mask_image":{"blob":1},)"
                     R"("strength":0.6)"),
               {Png, Mask}),
      E(Success));
  EXPECT_NE(hash(Id, 0), Plain);
  EXPECT_EQ(generate(Id, small(R"(,"vae_tiling":true)")), E(Success));
}

TEST_F(SDContractTest, TheSameRequestGivesTheSameImage) {
  const auto Id = session();
  ASSERT_EQ(generate(Id, small()), E(Success));
  const auto First = hash(Id, 0);
  ASSERT_EQ(generate(Id, small()), E(Success));
  EXPECT_EQ(hash(Id, 0), First);
}

TEST_F(SDContractTest, TheLargestClipSkipStillChangesTheImage) {
  const auto Id = session();
  ASSERT_EQ(generate(Id, small()), E(Success));
  const auto AllLayers = hash(Id, 0);
  ASSERT_EQ(generate(Id, small(",\"clip_skip\":" +
                               std::to_string(Limits::MaxClipSkip))),
            E(Success));
  EXPECT_NE(hash(Id, 0), AllLayers);
}

TEST_F(SDContractTest, EverySamplerAndSchedulerReturnsAtOneAndTwoSteps) {
  const auto Id = session();
  ASSERT_EQ(generate(Id, small()), E(Success));
  const auto Plain = hash(Id, 0);
  const auto Request = [](std::string_view Key, std::string_view Name,
                          uint32_t Steps) {
    return R"({"prompt":"a cat","width":64,"height":64,"seed":7,"steps":)" +
           std::to_string(Steps) + ",\"" + std::string(Key) +
           "\":" + jsonString(Name) + "}";
  };
  for (const auto Name : kSamplerNames) {
    for (const uint32_t Steps : {1U, 2U}) {
      EXPECT_EQ(generate(Id, Request("sampler"sv, Name, Steps)), E(Success))
          << "sampler " << Name << " at " << Steps << " steps";
    }
  }
  // No session has this id. The request check runs before the session lookup.
  // So a request below the minimum of a scheduler never reaches the library:
  // the check or the lookup refuses it. The known two-step schedulers always
  // use this id at one step, whatever the table says. Then a lowered table
  // entry cannot send them to the library.
  constexpr uint32_t kNoSession = UINT32_MAX;
  constexpr std::array<std::string_view, 3> kTwoStepSchedulers = {
      "karras"sv, "exponential"sv, "gits"sv};
  for (size_t I = 0; I < kSchedulerNames.size(); ++I) {
    const bool Known =
        std::find(kTwoStepSchedulers.begin(), kTwoStepSchedulers.end(),
                  kSchedulerNames[I]) != kTwoStepSchedulers.end();
    for (const uint32_t Steps : {1U, 2U}) {
      const auto Text = Request("scheduler"sv, kSchedulerNames[I], Steps);
      if (Steps < kSchedulerMinSteps[I] || (Known && Steps == 1)) {
        EXPECT_EQ(generate(kNoSession, Text), E(InvalidArgument))
            << "scheduler " << kSchedulerNames[I] << " at " << Steps
            << " steps";
      } else {
        EXPECT_EQ(generate(Id, Text), E(Success))
            << "scheduler " << kSchedulerNames[I] << " at " << Steps
            << " steps";
      }
    }
  }
  ASSERT_EQ(generate(Id, small()), E(Success));
  EXPECT_EQ(hash(Id, 0), Plain);
}

TEST_F(SDContractTest, OnlyThePluginsOwnLogLinesReachStdout) {
  const auto Id = session();
  const CapturedStdout Capture;
  ASSERT_EQ(generate(Id, small()), E(Success));
  EXPECT_EQ(Capture.linesWithout("[WasmEdge-StableDiffusion] "sv), 0U);
}

TEST_F(SDContractTest, NoPromptReachesTheHostLog) {
  writeSafetensors("narrow.safetensors", {{"emb_params", "F32", {2, 4}, 0.5F}});
  const auto Id = session(
      R"(,"embeddings":[{"name":"e9","path":"/aux/narrow.safetensors"}])");
  const auto Logger = spdlog::default_logger();
  const auto Level = Logger->level();
  Logger->set_level(spdlog::level::trace);
  const CapturedStdout Capture;
  spdlog::trace("zvprobe"sv);
  const auto Negative = R"(,"negative_prompt":"a zvnegq blur")"sv;
  EXPECT_EQ(
      generate(Id, small(Negative, "a zvposq cat, (zvposq:.) <lora:zvposq:y>")),
      E(Success));
  EXPECT_EQ(generate(Id, small(Negative, "a zvposq e9 cat")), E(RuntimeError));
  Logger->flush();
  Logger->set_level(Level);
  EXPECT_TRUE(Capture.holds("zvprobe"sv));
  EXPECT_FALSE(Capture.holds("zvposq"sv));
  EXPECT_FALSE(Capture.holds("zvnegq"sv));
}

} // namespace
