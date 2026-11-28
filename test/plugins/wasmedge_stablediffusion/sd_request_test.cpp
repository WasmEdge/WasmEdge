// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "sd_log.h"
#include "sd_request.h"

#include "common/spdlog.h"

#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace {

using namespace std::literals;
using namespace WasmEdge::Host::StableDiffusion;

TEST(SDRequestTest, ParsesAFullRequest) {
  auto R = parseGenerationRequest(
      R"({"prompt":"a cat","negative_prompt":"blur","width":768,"height":512,
          "seed":42,"steps":30,"cfg_scale":7.5,"guidance":3.5,
          "control_strength":0.9,"strength":0.75,"sampler":"euler_a",
          "scheduler":"karras","clip_skip":2,"batch_count":2,
          "init_image":{"blob":0},"mask_image":{"blob":1},
          "control_image":{"blob":2},
          "loras":[{"path":"/l/a.safetensors","multiplier":0.8}],
          "vae_tiling":true,"upscale_repeats":1})"sv,
      3);
  ASSERT_TRUE(R);
  EXPECT_EQ(R->Prompt, "a cat");
  EXPECT_EQ(R->Width, 768U);
  EXPECT_EQ(R->Seed, 42);
  EXPECT_EQ(R->Sampler, std::optional<uint8_t>(1));
  EXPECT_EQ(R->Scheduler, std::optional<uint8_t>(1));
  EXPECT_EQ(R->MaskImage, std::optional<uint32_t>(1));
  ASSERT_EQ(R->Loras.size(), 1U);
  EXPECT_FLOAT_EQ(R->Loras[0].Multiplier, 0.8F);
  EXPECT_TRUE(R->VaeTiling);
  EXPECT_EQ(R->UpscaleRepeats, 1U);
}

TEST(SDRequestTest, AppliesDefaults) {
  auto R = parseGenerationRequest(R"({"prompt":"a"})"sv, 0);
  ASSERT_TRUE(R);
  EXPECT_EQ(R->Width, 512U);
  EXPECT_EQ(R->Height, 512U);
  EXPECT_LT(R->Seed, 0);
  EXPECT_EQ(R->Steps, 20U);
  EXPECT_EQ(R->BatchCount, 1U);
  EXPECT_FALSE(R->CfgScale);
  EXPECT_FALSE(R->Sampler);
  EXPECT_EQ(R->UpscaleRepeats, 0U);
}

TEST(SDRequestTest, ParsesASessionConfig) {
  auto C = parseSessionConfig(
      R"({"model_path":"/m/sd.safetensors","vae_path":"/m/vae.safetensors",
          "embeddings":[{"name":"e1","path":"/m/e1.safetensors"}],
          "n_threads":4,"weight_type":"q8_0","rng":"cpu",
          "flash_attention":true,"device":"cpu",
          "cpu_modules":["vae","text_encoder"]})"sv);
  ASSERT_TRUE(C);
  EXPECT_EQ(C->ModelPath, "/m/sd.safetensors");
  EXPECT_EQ(C->Threads, 4U);
  EXPECT_EQ(C->WeightType, std::optional<uint8_t>(3));
  EXPECT_EQ(C->Rng, std::optional<uint8_t>(2));
  EXPECT_EQ(C->Device, 1U);
  EXPECT_EQ(C->CpuModules, 0b011U);
  ASSERT_EQ(C->Embeddings.size(), 1U);
}

struct RejectCase {
  std::string_view Json;
  uint32_t Blobs;
  ErrNo Expected;
};

TEST(SDRequestTest, RejectsBadRequests) {
  static const RejectCase Cases[] = {
      {""sv, 0, ErrNo::InvalidEncoding},
      {"[]"sv, 0, ErrNo::InvalidEncoding},
      {"{\"prompt\":\"a\""sv, 0, ErrNo::InvalidEncoding},
      {"{\"prompt\":\"\xff\"}"sv, 0, ErrNo::InvalidEncoding},
      {R"({})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","nope":1})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","prompt":"b"})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":1})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a\u0000b"})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","width":0})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","width":100})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","width":8256})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","width":8192,"height":4160})"sv, 0,
       ErrNo::InvalidArgument},
      {R"({"prompt":"a","steps":0})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","steps":1001})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","steps":4294967316})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","cfg_scale":-0.5})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","cfg_scale":1000.5})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","cfg_scale":1e39})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","strength":1.5})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","control_strength":100.5})"sv, 0,
       ErrNo::InvalidArgument},
      {R"({"prompt":"a","clip_skip":0})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","clip_skip":13})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","batch_count":17})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","batch_count":4294967297})"sv, 0,
       ErrNo::InvalidArgument},
      {R"({"prompt":"a","sampler":"auto"})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","scheduler":"Karras"})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","init_image":{"blob":0}})"sv, 0,
       ErrNo::InvalidArgument},
      {R"({"prompt":"a","mask_image":{"blob":0}})"sv, 1,
       ErrNo::InvalidArgument},
      {R"({"prompt":"a","init_image":{"blob":0,"x":1}})"sv, 1,
       ErrNo::InvalidArgument},
      {R"({"prompt":"a","init_image":{"blob":0,"blob":0}})"sv, 1,
       ErrNo::InvalidArgument},
      {R"({"prompt":"a","loras":[{"path":"/l/a","multiplier":100.5}]})"sv, 0,
       ErrNo::InvalidArgument},
      {R"({"prompt":"a","loras":[{"path":""}]})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","loras":[{"path":"/l/a","path":"/l/b"}]})"sv, 0,
       ErrNo::InvalidArgument},
      {R"({"prompt":"a","upscale_repeats":0})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","upscale_repeats":4})"sv, 0, ErrNo::InvalidArgument},
      {R"({"prompt":"a","vae_tiling":1})"sv, 0, ErrNo::InvalidArgument},
  };
  for (const auto &C : Cases) {
    auto R = parseGenerationRequest(C.Json, C.Blobs);
    ASSERT_FALSE(R) << C.Json;
    EXPECT_EQ(R.error(), C.Expected) << C.Json;
  }
}

TEST(SDRequestTest, AcceptsTheEdges) {
  for (const auto Json : {
           R"({"prompt":"a","width":64,"height":64})"sv,
           R"({"prompt":"a","width":8192,"height":4096})"sv,
           R"({"prompt":"a","steps":1})"sv,
           R"({"prompt":"a","steps":1000})"sv,
           R"({"prompt":"a","cfg_scale":0})"sv,
           R"({"prompt":"a","cfg_scale":1000})"sv,
           R"({"prompt":"a","clip_skip":12,"batch_count":16})"sv,
           R"({"prompt":"a","seed":-1})"sv,
           R"({"prompt":"a","seed":9223372036854775807})"sv,
           R"({"prompt":"","negative_prompt":""})"sv,
       }) {
    EXPECT_TRUE(parseGenerationRequest(Json, 0)) << Json;
  }
}

TEST(SDRequestTest, ASchedulerTakesAtLeastItsMinimumOfSteps) {
  const auto At = [](std::string_view Scheduler, uint32_t Steps) {
    return parseGenerationRequest(
        "{\"prompt\":\"a\",\"scheduler\":\"" + std::string(Scheduler) +
            "\",\"steps\":" + std::to_string(Steps) + "}",
        0);
  };
  const auto TwoSteps = {"karras"sv, "exponential"sv, "gits"sv};
  for (const auto Name : TwoSteps) {
    const auto One = At(Name, 1);
    ASSERT_FALSE(One) << Name;
    EXPECT_EQ(One.error(), ErrNo::InvalidArgument) << Name;
    EXPECT_TRUE(At(Name, 2)) << Name;
  }
  for (const auto Name : kSchedulerNames) {
    if (std::find(TwoSteps.begin(), TwoSteps.end(), Name) == TwoSteps.end()) {
      EXPECT_TRUE(At(Name, 1)) << Name;
    }
  }
  EXPECT_EQ(parseGenerationRequest(
                R"({"prompt":"a","steps":1,"scheduler":"gits"})"sv, 0)
                .error(),
            ErrNo::InvalidArgument);
  EXPECT_TRUE(parseGenerationRequest(R"({"prompt":"a","steps":1})"sv, 0));

  std::ostringstream Out;
  auto Logger = std::make_shared<spdlog::logger>(
      "sd-request-steps",
      std::make_shared<spdlog::sinks::ostream_sink_mt>(Out));
  Logger->set_pattern("%l|%v");
  const auto Previous = spdlog::default_logger();
  spdlog::set_default_logger(Logger);
  EXPECT_FALSE(At("gits"sv, 1));
  spdlog::set_default_logger(Previous);
  spdlog::drop("sd-request-steps");
  EXPECT_EQ(Out.str(), "error|[WasmEdge-StableDiffusion] generation request: "
                       "steps: must be at least 2 with the scheduler "
                       "\"gits\"\n");
}

TEST(SDRequestTest, EnforcesTheTextAndPromptLimits) {
  const std::string Half(750, 'a');
  EXPECT_TRUE(parseGenerationRequest(
      "{\"prompt\":\"" + Half + "\",\"negative_prompt\":\"" + Half + "\"}", 0));
  EXPECT_EQ(parseGenerationRequest("{\"prompt\":\"" + Half +
                                       "a\",\"negative_prompt\":\"" + Half +
                                       "\"}",
                                   0)
                .error(),
            ErrNo::InvalidArgument);
  const std::string Big = "{\"prompt\":\"a\"" + std::string(65536, ' ') + "}";
  EXPECT_EQ(parseGenerationRequest(Big, 0).error(), ErrNo::InvalidArgument);
}

TEST(SDRequestTest, RejectsBadConfigs) {
  for (
      const auto Json : {
          R"({})"sv,
          R"({"model_path":""})"sv,
          R"({"model_path":"/m","n_threads":0})"sv,
          R"({"model_path":"/m","n_threads":1025})"sv,
          R"({"model_path":"/m","weight_type":"q4_K"})"sv,
          R"({"model_path":"/m","rng":"gpu"})"sv,
          R"({"model_path":"/m","device":"cuda"})"sv,
          R"({"model_path":"/m","cpu_modules":["unet"]})"sv,
          R"({"model_path":"/m","cpu_modules":["vae","vae"]})"sv,
          R"({"model_path":"/m","embeddings":[{"name":"e"}]})"sv,
          R"({"model_path":"/m","embeddings":[{"name":"","path":"/e"}]})"sv,
          R"({"model_path":"/m","embeddings":[{"name":"e","path":"/e","x":1}]})"sv,
          R"({"model_path":"/m","embeddings":{"name":"e","path":"/e"}})"sv,
          R"({"model_path":"/m","backend":"cpu"})"sv,
      }) {
    auto C = parseSessionConfig(Json);
    ASSERT_FALSE(C) << Json;
    EXPECT_EQ(C.error(), ErrNo::InvalidArgument) << Json;
  }
  std::string Many = R"({"model_path":"/m","embeddings":[)";
  for (int I = 0; I < 65; ++I) {
    Many += (I ? "," : "") + std::string(R"({"name":"e","path":"/e"})");
  }
  EXPECT_EQ(parseSessionConfig(Many + "]}").error(), ErrNo::InvalidArgument);
  EXPECT_TRUE(parseSessionConfig(R"({"diffusion_model_path":"/d"})"sv));
}

TEST(SDRequestTest, IntegerKeysRejectFloatsAndStrings) {
  for (const auto Json : {
           R"({"prompt":"a","steps":20.0})"sv,
           R"({"prompt":"a","seed":1e3})"sv,
           R"({"prompt":"a","width":"512"})"sv,
           R"({"prompt":"a","batch_count":1.5})"sv,
           R"({"prompt":"a","seed":18446744073709551615})"sv,
           R"({"prompt":"a","init_image":{"blob":0.0}})"sv,
       }) {
    auto R = parseGenerationRequest(Json, 1);
    ASSERT_FALSE(R) << Json;
    EXPECT_EQ(R.error(), ErrNo::InvalidArgument) << Json;
  }
  EXPECT_TRUE(parseGenerationRequest(R"({"prompt":"a","cfg_scale":7})"sv, 0));
}

TEST(SDRequestTest, ValidJsonTheParserRefusesGetsTheDocumentedCodes) {
  // A request with Levels nesting levels, the outer object included.
  const auto Nested = [](size_t Levels) {
    return "{\"prompt\":\"a\",\"seed\":" + std::string(Levels - 1, '[') +
           std::string(Levels - 1, ']') + "}";
  };
  for (const auto &Json :
       {std::string(R"({"prompt":"a","cfg_scale":1e400})"),
        std::string(R"({"prompt":"a","seed":-1e400})"), Nested(1025)}) {
    const auto R = parseGenerationRequest(Json, 0);
    ASSERT_FALSE(R) << Json.substr(0, 40);
    EXPECT_EQ(R.error(), ErrNo::InvalidEncoding) << Json.substr(0, 40);
  }
  for (const auto &Json :
       {std::string(R"({"prompt":"a","seed":18446744073709551616})"),
        std::string(R"({"prompt":"a","seed":-18446744073709551617})"),
        std::string(R"({"prompt":"a","cfg_scale":100000000000000000000})"),
        std::string(R"({"prompt":"a","x":[{"y":18446744073709551616}]})"),
        Nested(1024)}) {
    const auto R = parseGenerationRequest(Json, 0);
    ASSERT_FALSE(R) << Json.substr(0, 40);
    EXPECT_EQ(R.error(), ErrNo::InvalidArgument) << Json.substr(0, 40);
  }
  EXPECT_EQ(
      parseSessionConfig(R"({"model_path":"/m","n_threads":1e400})"sv).error(),
      ErrNo::InvalidEncoding);
  EXPECT_EQ(parseSessionConfig(
                R"({"model_path":"/m","n_threads":18446744073709551616})"sv)
                .error(),
            ErrNo::InvalidArgument);
}

TEST(SDRequestTest, LogTextCannotBreakALogLine) {
  EXPECT_EQ(logText("model_path"sv), "model_path");
  EXPECT_EQ(logText("a\nb\x1b[31m\tc"sv), "a?b?[31m?c");
  EXPECT_EQ(logText(std::string(64, 'k')), std::string(64, 'k'));
  EXPECT_EQ(logText(std::string(65, 'k')), std::string(64, 'k') + "...");
  EXPECT_EQ(logText("\xe4\xb8\xad"sv), "???");
  EXPECT_EQ(logText(std::string_view("a\0b", 3)), "a?b");
  EXPECT_EQ(logText(""sv), "");
}

TEST(SDRequestTest, ARejectionLogsOneLineWithoutTheGuestsLineBreaks) {
  std::ostringstream Out;
  auto Logger = std::make_shared<spdlog::logger>(
      "sd-request", std::make_shared<spdlog::sinks::ostream_sink_mt>(Out));
  Logger->set_pattern("%l|%v");
  const auto Previous = spdlog::default_logger();
  spdlog::set_default_logger(Logger);
  const auto R = parseGenerationRequest(
      "{\"prompt\":\"secret words\",\"evil\\nkey\\u001b[2J\":1}"sv, 0);
  spdlog::set_default_logger(Previous);
  spdlog::drop("sd-request");
  ASSERT_FALSE(R);
  EXPECT_EQ(R.error(), ErrNo::InvalidArgument);
  const auto Text = Out.str();
  EXPECT_EQ(std::count(Text.begin(), Text.end(), '\n'), 1);
  EXPECT_EQ(Text.rfind("error|[WasmEdge-StableDiffusion] ", 0), 0U);
  EXPECT_NE(Text.find("evil?key?[2J"), std::string::npos);
  EXPECT_EQ(Text.find('\x1b'), std::string::npos);
  EXPECT_EQ(Text.find("secret words"), std::string::npos);
}

} // namespace
