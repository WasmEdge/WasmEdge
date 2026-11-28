// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "safetensors_fixture.h"
#include "sd_adapter_check.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unistd.h>

namespace {

using namespace std::literals;
using namespace WasmEdge::Host::StableDiffusion;
using namespace Fixture;

std::string padWithSpaces(std::string_view Body, uint64_t Length) {
  return std::string(Body) + std::string(Length - Body.size(), ' ');
}

TEST(SDAdapterCheckTest, AcceptsThePlainLoraFamily) {
  for (const auto &H : {
           lora("[2,320]", "[320,2]"),
           lora("[2,320,3,3]", "[320,2,1,1]"),
           lora("[1,320]", "[320,1]"),
           lora("[1,320,3,3]", "[320,1,1,1]"),
           lora("[1,1,1,3]", "[1,1,1,1]"),
           lora("[2,320]", "[320,2]", "," + tensor("m.alpha", "F32", "[]")),
           lora("[2,320]", "[320,2]", "," + tensor("m.alpha", "F16", "[1,1]")),
           lora("[2,320]", "[320,2]",
                ",\"__metadata__\":{\"alpha\":\"4\",\"ss_network_dim\":\"2\"}"),
           "{" + tensor("m.lora_A.weight", "BF16", "[4,768]") + "," +
               tensor("m.lora_B.weight", "BF16", "[768,4]") + "}",
           "{\"__metadata__\":{\"format\":\"pt\"}," +
               tensor("m.lora_down.weight", "F16", "[2,320]") + "," +
               tensor("m.lora_up.weight", "F16", "[320,2]") + "}",
           "{" + tensor("a.lora_down.weight", "F32", "[2,320]") + "," +
               tensor("a.lora_up.weight", "F32", "[320,2]") + "," +
               tensor("b.lora_A.weight", "F32", "[8,640]") + "," +
               tensor("b.lora_B.weight", "F32", "[640,8]") + "}",
       }) {
    EXPECT_EQ(checkAdapterHeader(AdapterKind::Lora, 1U << 20, H),
              ErrNo::Success)
        << H;
  }
}

TEST(SDAdapterCheckTest, RefusesTheLayoutsTheLibraryAssertsOn) {
  for (const auto &H : {
           lora("[2,320]", "[320,3]"),
           lora("[0,320]", "[320,2]"),
           lora("[2,320]", "[0,2]"),
           lora("[640]", "[640]"),
           lora("[2,320,1,1]", "[1,2,1,1]"),
           lora("[2,320,3,3]", "[1,2,1,1]"),
           lora("[1,1,3,3]", "[320,1,1,1]"),
           lora("[2,320]", "[320,2]", "," + tensor("m.alpha", "F32", "[3]")),
           "{" + tensor("m.lora_down.weight", "I64", "[2,320]") + "," +
               tensor("m.lora_up.weight", "I64", "[320,2]") + "}",
       }) {
    EXPECT_EQ(checkAdapterHeader(AdapterKind::Lora, 1U << 20, H),
              ErrNo::LoadFailed)
        << H;
  }
  const auto Big = "{" + tensor("emb", "F32", "[35000,768]") + "}";
  EXPECT_EQ(
      checkAdapterHeader(AdapterKind::Embedding, UINT64_C(107520080), Big),
      ErrNo::LoadFailed);
}

TEST(SDAdapterCheckTest, RefusesWhatItHasNoRuleFor) {
  for (const auto &H : {
           "{" + tensor("m.lokr_w1", "F32", "[4,4]") + "}",
           "{" + tensor("m.hada_w1_a", "F32", "[4,4]") + "}",
           "{" + tensor("m.lora_mid.weight", "F32", "[2,2,3,3]") + "}",
           "{" + tensor("m.diff", "F32", "[320,320]") + "}",
           "{" + tensor("m.LORA_DOWN.weight", "F32", "[2,320]") + "," +
               tensor("m.LORA_UP.weight", "F32", "[320,2]") + "}",
           lora("[2,320,3]", "[320,2]"),
           lora("[2,320]", "[320,2,3,3]"),
           lora("[2,-1]", "[320,2]"),
           lora("[2,\"x\"]", "[320,2]"),
           std::string("[]"),
           std::string("not json"),
           std::string("{\"m.lora_down.weight\":7}"),
           std::string("{}"),
           std::string("{\"__metadata__\":{\"format\":\"pt\"}}"),
           "{" + tensor("m.lora_down.weight", "F32", "[2,320]") + "}",
           "{" + tensor("m.lora_up.weight", "F32", "[320,2]") + "}",
           "{" + tensor("m.alpha", "F32", "[]") + "}",
           lora("[2,320]", "[320,2]",
                "," + tensor("m.lora_A.weight", "F32", "[2,320]")),
           lora("[2,320]", "[320,2]", "," + tensor("n.alpha", "F32", "[]")),
           "{" + tensor("m.lora_down.weight", "F32", "[2,320]") + "," +
               tensor("m.lora_down.weight", "F32", "[3,320]") + "," +
               tensor("m.lora_up.weight", "F32", "[320,2]") + "}",
           "{" + tensor(".lora_down.weight", "F32", "[2,320]") + "," +
               tensor(".lora_up.weight", "F32", "[320,2]") + "}",
           "{" + tensor("m.lora_down.weight", "F32", "[2,320]") + "," +
               tensor("m.lora_B.weight", "F32", "[320,2]") + "}",
           "{" + tensor("m.lora_A.weight", "F32", "[2,320]") + "," +
               tensor("m.lora_up.weight", "F32", "[320,2]") + "}",
           "{\"m.lora_down.weight\":{\"dtype\":\"F32\",\"dtype\":\"F32\","
           "\"shape\":[2,320]}," +
               tensor("m.lora_up.weight", "F32", "[320,2]") + "}",
           "{\"__metadata__\":{\"a\":\"1\"},\"__metadata__\":{\"b\":\"2\"}," +
               tensor("m.lora_down.weight", "F32", "[2,320]") + "," +
               tensor("m.lora_up.weight", "F32", "[320,2]") + "}",
           lora("[2,320]", "[320,2]", ",\"m.alpha\":{\"shape\":[]}"),
           lora("[2,320]", "[320,2]",
                ",\"m.alpha\":{\"dtype\":32,\"shape\":[]}"),
           lora("[2,320]", "[320,2]", ",\"m.alpha\":{\"dtype\":\"F32\"}"),
           lora("[2,320]", "[320,2]", "," + tensor("m.alpha", "F32", "1")),
           lora("[2,320]", "[320,2]",
                ",\"__metadata__\":{\"adapter_config\":\"{}\"}"),
           lora("[2,320]", "[320,2]", ",\"__metadata__\":{\"k\":1}"),
           lora("[2,320]", "[320,2]", ",\"__metadata__\":7"),
           lora("[2,320]", "[320,2]",
                ",\"__metadata__\":{\"k\":\"1\",\"k\":\"2\"}"),
           lora("[2.0,320]", "[320,2]"),
       }) {
    EXPECT_EQ(checkAdapterHeader(AdapterKind::Lora, 1U << 20, H),
              ErrNo::LoadFailed)
        << H;
  }
}

TEST(SDAdapterCheckTest, RefusesATensorLargerThanItsFile) {
  const auto H = lora("[2,320]", "[320,2]");
  const uint64_t Fits = 8 + H.size() + 5120;
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Lora, Fits, H), ErrNo::Success);
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Lora, Fits - 1, H),
            ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterHeader(
                AdapterKind::Lora, UINT64_MAX,
                lora("[2,4611686018427387904]", "[4611686018427387904,2]")),
            ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterHeader(
                AdapterKind::Lora, UINT64_MAX,
                lora("[4294967296,4294967296]", "[4294967296,4294967296]")),
            ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Embedding, 6143,
                               "{" + tensor("e", "F32", "[2,768]") + "}"),
            ErrNo::LoadFailed);
}

// A header entry with its data at [Begin, Begin + Bytes) of the data section.
std::string placed(std::string_view Name, std::string_view Dtype,
                   std::string_view Shape, uint64_t Begin, uint64_t Bytes) {
  return "\"" + std::string(Name) + "\":{\"dtype\":\"" + std::string(Dtype) +
         "\",\"shape\":" + std::string(Shape) + ",\"data_offsets\":[" +
         std::to_string(Begin) + "," + std::to_string(Begin + Bytes) + "]}";
}

TEST(SDAdapterCheckTest, RefusesTensorsThatTogetherDoNotFitTheFile) {
  const auto Size = [](const std::string &Header, uint64_t Data) {
    return 8 + Header.size() + Data;
  };
  std::string Shared = "{";
  std::string Honest = "{";
  uint64_t Index = 0;
  for (const auto Name : {"e1"sv, "e2"sv, "e3"sv, "e4"sv}) {
    const char *Comma = Index == 0 ? "" : ",";
    Shared += Comma + placed(Name, "F16", "[2,768]", 0, 3072);
    Honest += Comma + placed(Name, "F16", "[2,768]", Index * 3072, 3072);
    ++Index;
  }
  Shared += "}";
  Honest += "}";
  EXPECT_EQ(
      checkAdapterHeader(AdapterKind::Embedding, Size(Shared, 3072), Shared),
      ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Embedding, Size(Honest, 4 * 3072),
                               Honest),
            ErrNo::Success);

  const auto Down = "m.lora_down.weight"sv;
  const auto Up = "m.lora_up.weight"sv;
  const auto SharedLora = "{" + placed(Down, "F32", "[2,320]", 0, 2560) + "," +
                          placed(Up, "F32", "[320,2]", 0, 2560) + "}";
  const auto HonestLora = "{" + placed(Down, "F32", "[2,320]", 0, 2560) + "," +
                          placed(Up, "F32", "[320,2]", 2560, 2560) + "}";
  EXPECT_EQ(
      checkAdapterHeader(AdapterKind::Lora, Size(SharedLora, 2560), SharedLora),
      ErrNo::LoadFailed);
  EXPECT_EQ(
      checkAdapterHeader(AdapterKind::Lora, Size(HonestLora, 5120), HonestLora),
      ErrNo::Success);
}

TEST(SDAdapterCheckTest, EmbeddingRules) {
  const auto Emb = "{" + tensor("emb_params", "F32", "[2,768]") + "}";
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Embedding, 6400, Emb),
            ErrNo::Success);
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Embedding,
                               Limits::MaxEmbeddingFileBytes, Emb),
            ErrNo::Success);
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Embedding,
                               Limits::MaxEmbeddingFileBytes + 1, Emb),
            ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Embedding, 6400,
                               "{" + tensor("e", "I64", "[2,768]") + "}"),
            ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Embedding, 6400, "{}"),
            ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Embedding, 1U << 20,
                               "{" + tensor("e", "F32", "[2,2,768]") + "}"),
            ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Embedding, 1U << 20,
                               "{" + tensor("e", "F16", "[1,2,768]") + "}"),
            ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Embedding, 1U << 20,
                               "{" + tensor("e", "F32", "[]") + "}"),
            ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Embedding, 1U << 20,
                               "{" + tensor("e", "F32", "[2,768]") + "," +
                                   tensor("\\u0065", "F32", "[2,768]") + "}"),
            ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Embedding, 1U << 20,
                               "{" + tensor("e", "F32", "[768]") + "}"),
            ErrNo::Success);
}

TEST(SDAdapterCheckTest, TellsTheRowsOneMentionOfAnEmbeddingStandsFor) {
  const auto Rows = [](const std::string &Header) {
    uint64_t Found = 0;
    EXPECT_EQ(
        checkAdapterHeader(AdapterKind::Embedding, 1U << 24, Header, &Found),
        ErrNo::Success)
        << Header;
    return Found;
  };
  EXPECT_EQ(Rows("{" + tensor("e", "F32", "[768]") + "}"), 1U);
  EXPECT_EQ(Rows("{" + tensor("e", "F32", "[1,768]") + "}"), 1U);
  EXPECT_EQ(Rows("{" + tensor("e", "F16", "[75,768]") + "}"), 75U);
  EXPECT_EQ(Rows("{" + tensor("clip_l", "F32", "[8,768]") + "," +
                 tensor("clip_g", "F32", "[12,1280]") + "," +
                 tensor("flat", "F32", "[768]") + "}"),
            12U);
  uint64_t Untouched = 7;
  EXPECT_EQ(checkAdapterHeader(AdapterKind::Embedding, 1U << 24,
                               "{" + tensor("e", "F32", "[2,2,768]") + "}",
                               &Untouched),
            ErrNo::LoadFailed);
  EXPECT_EQ(Untouched, 7U);
}

TEST(SDAdapterCheckTest, KnowsAnUnquantizedSafetensorsModel) {
  const auto Dir = std::filesystem::temp_directory_path() /
                   ("wasmedge-sd-model-" + std::to_string(::getpid()));
  std::filesystem::create_directories(Dir);
  const auto Plain = "{" + tensor("model.w", "F16", "[4,4]") + "," +
                     tensor("model.ids", "I64", "[1,77]") + "}";
  EXPECT_TRUE(isUnquantizedSafetensors(
      writeHeaderFile(Dir / "plain", Plain.size(), Plain, 1024)));
  const auto Quant = "{" + tensor("model.w", "I8", "[4,4]") + "," +
                     tensor("model.w.comfy_quant", "U8", "[16]") + "}";
  EXPECT_FALSE(isUnquantizedSafetensors(
      writeHeaderFile(Dir / "quant", Quant.size(), Quant, 1024)));
  for (const auto Dtype : {"F8_E4M3"sv, "F8_E5M2"sv}) {
    const auto Fp8 = "{" + tensor("model.w", "F16", "[4,4]") + "," +
                     tensor("model.v", Dtype, "[4,4]") + "}";
    EXPECT_FALSE(isUnquantizedSafetensors(
        writeHeaderFile(Dir / std::string(Dtype), Fp8.size(), Fp8, 1024)))
        << Dtype;
  }
  EXPECT_FALSE(isUnquantizedSafetensors(
      writeHeaderFile(Dir / "lying", Plain.size() + 4096, Plain)));
  EXPECT_FALSE(
      isUnquantizedSafetensors(writeHeaderFile(Dir / "array", 2, "[]", 1024)));
  {
    std::ofstream Gguf(Dir / "model.gguf", std::ios::binary);
    Gguf << "GGUF" << std::string(4096, '\0');
  }
  EXPECT_FALSE(isUnquantizedSafetensors(Dir / "model.gguf"));
  {
    std::ofstream Empty(Dir / "empty", std::ios::binary);
  }
  EXPECT_FALSE(isUnquantizedSafetensors(Dir / "empty"));
  EXPECT_FALSE(isUnquantizedSafetensors(Dir));
  EXPECT_FALSE(isUnquantizedSafetensors(Dir / "missing"));
  const auto AtLimit = padWithSpaces(Plain, Limits::MaxSafetensorsHeaderBytes);
  const auto OverLimit =
      padWithSpaces(Plain, Limits::MaxSafetensorsHeaderBytes + 1);
  EXPECT_TRUE(isUnquantizedSafetensors(
      writeHeaderFile(Dir / "at-limit", AtLimit.size(), AtLimit, 1024)));
  EXPECT_FALSE(isUnquantizedSafetensors(
      writeHeaderFile(Dir / "over-limit", OverLimit.size(), OverLimit, 1024)));
  std::filesystem::remove_all(Dir);
}

TEST(SDAdapterCheckTest, ReadsTheHeaderFromAFile) {
  const auto Dir = std::filesystem::temp_directory_path() /
                   ("wasmedge-sd-check-" + std::to_string(::getpid()));
  std::filesystem::create_directories(Dir);
  const auto Write = [&](const char *Name, uint64_t Length,
                         std::string_view Body, uint64_t Payload = 0) {
    return writeHeaderFile(Dir / Name, Length, Body, Payload);
  };
  const auto Good = lora("[2,320]", "[320,2]");
  EXPECT_EQ(checkAdapterFile(AdapterKind::Lora,
                             Write("good", Good.size(), Good, 5120)),
            ErrNo::Success);
  EXPECT_EQ(
      checkAdapterFile(AdapterKind::Lora, Write("no-data", Good.size(), Good)),
      ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterFile(AdapterKind::Lora,
                             Write("short-data", Good.size(), Good, 5119)),
            ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterFile(AdapterKind::Lora, Write("lying", 1U << 30, Good)),
            ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterFile(AdapterKind::Lora, Write("zero", 0, Good)),
            ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterFile(AdapterKind::Lora,
                             Write("huge", UINT64_C(1) << 62, Good)),
            ErrNo::LoadFailed);
  EXPECT_EQ(
      checkAdapterFile(AdapterKind::Lora, Write("past", Good.size() + 1, Good)),
      ErrNo::LoadFailed);
  const auto AtLimit = padWithSpaces(Good, Limits::MaxSafetensorsHeaderBytes);
  const auto OverLimit =
      padWithSpaces(Good, Limits::MaxSafetensorsHeaderBytes + 1);
  EXPECT_EQ(checkAdapterFile(AdapterKind::Lora,
                             Write("at-limit", AtLimit.size(), AtLimit, 5120)),
            ErrNo::Success);
  EXPECT_EQ(
      checkAdapterFile(AdapterKind::Lora,
                       Write("over-limit", OverLimit.size(), OverLimit, 5120)),
      ErrNo::LoadFailed);
  {
    std::ofstream Short(Dir / "short", std::ios::binary);
    Short << "abc";
  }
  EXPECT_EQ(checkAdapterFile(AdapterKind::Lora, Dir / "short"),
            ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterFile(AdapterKind::Lora, Dir), ErrNo::LoadFailed);
  EXPECT_EQ(checkAdapterFile(AdapterKind::Lora, Dir / "missing"),
            ErrNo::LoadFailed);
  std::filesystem::remove_all(Dir);
}

} // namespace
