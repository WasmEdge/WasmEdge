// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "sd_abi.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace {

using namespace std::literals;
using namespace WasmEdge::Host::StableDiffusion;

TEST(SDAbiTest, ErrorCodesAndVersionAreTheDocumentedNumbers) {
  EXPECT_EQ(static_cast<uint32_t>(ErrNo::Success), 0U);
  EXPECT_EQ(static_cast<uint32_t>(ErrNo::Busy), 4U);
  EXPECT_EQ(static_cast<uint32_t>(ErrNo::BufferTooSmall), 6U);
  EXPECT_EQ(static_cast<uint32_t>(ErrNo::NotFound), 7U);
  EXPECT_EQ(static_cast<uint32_t>(ErrNo::LoadFailed), 8U);
  EXPECT_EQ(static_cast<uint32_t>(ErrNo::NotReadOnly), 9U);
  EXPECT_EQ(kAbiVersion, 2U);
  EXPECT_EQ(static_cast<uint32_t>(ResultFormat::Rgb8), 1U);
}

TEST(SDAbiTest, LookupIsExact) {
  EXPECT_EQ(indexOf(kSamplerNames, "euler"sv), std::optional<uint8_t>(0));
  EXPECT_EQ(indexOf(kSamplerNames, "lms"sv), std::optional<uint8_t>(20));
  EXPECT_EQ(indexOf(kSchedulerNames, "llada_image"sv),
            std::optional<uint8_t>(16));
  EXPECT_EQ(indexOf(kWeightTypeNames, "q4_k"sv), std::optional<uint8_t>(8));
  EXPECT_EQ(indexOf(kRngNames, "cpu"sv), std::optional<uint8_t>(2));
  EXPECT_FALSE(indexOf(kSamplerNames, "Euler"sv));
  EXPECT_FALSE(indexOf(kSamplerNames, "auto"sv));
  EXPECT_FALSE(indexOf(kSamplerNames, ""sv));
  EXPECT_FALSE(indexOf(kSamplerNames, "euler "sv));
}

template <size_t N>
void expectEveryNameAtItsIndex(std::string_view Label,
                               const std::array<std::string_view, N> &Table) {
  SCOPED_TRACE(Label);
  for (size_t I = 0; I < N; ++I) {
    EXPECT_FALSE(Table[I].empty()) << "index " << I;
    EXPECT_EQ(std::count(Table.begin(), Table.end(), Table[I]), 1) << Table[I];
    EXPECT_EQ(indexOf(Table, Table[I]),
              std::optional<uint8_t>(static_cast<uint8_t>(I)))
        << Table[I];
  }
}

TEST(SDAbiTest, EveryNameIsFoundAtItsOwnIndex) {
  expectEveryNameAtItsIndex("kSamplerNames"sv, kSamplerNames);
  expectEveryNameAtItsIndex("kSchedulerNames"sv, kSchedulerNames);
  expectEveryNameAtItsIndex("kWeightTypeNames"sv, kWeightTypeNames);
  expectEveryNameAtItsIndex("kRngNames"sv, kRngNames);
  expectEveryNameAtItsIndex("kDeviceNames"sv, kDeviceNames);
  expectEveryNameAtItsIndex("kCpuModuleNames"sv, kCpuModuleNames);
}

TEST(SDAbiTest, KarrasExponentialAndGitsTakeTwoStepsAndTheRestOne) {
  for (size_t I = 0; I < kSchedulerNames.size(); ++I) {
    const auto Name = kSchedulerNames[I];
    const bool Two =
        Name == "karras"sv || Name == "exponential"sv || Name == "gits"sv;
    EXPECT_EQ(kSchedulerMinSteps[I], Two ? 2U : 1U) << Name;
  }
}

} // namespace
