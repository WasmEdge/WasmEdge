// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "sd_abi.h"
#include "sd_image.h"
#include "test_support.h"

#include "common/types.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/function.h"
#include "runtime/instance/module.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#define E(x) static_cast<uint32_t>(ErrNo::x)

namespace {

using namespace std::literals;
using WasmEdge::Host::StableDiffusion::ErrNo;
using WasmEdge::Host::StableDiffusion::loadPlugin;
using WasmEdge::Host::StableDiffusion::SDContractTest;

TEST(WasmEdgeStableDiffusionTest, ThePluginLoadsAndReportsItsAbiVersion) {
  auto Mod = loadPlugin();
  ASSERT_TRUE(Mod);
  auto *Fn = Mod->findFuncExports("abi_version");
  ASSERT_NE(Fn, nullptr);
  WasmEdge::Runtime::Instance::ModuleInstance Caller("");
  WasmEdge::Runtime::CallingFrame Frame(nullptr, &Caller);
  std::array<WasmEdge::ValVariant, 1> Ret = {UINT32_C(0)};
  ASSERT_TRUE(Fn->getHostFunc().run(Frame, {}, Ret));
  EXPECT_EQ(Ret[0].get<uint32_t>(), 2U);
  EXPECT_EQ(Mod->findFuncExports("create_context"), nullptr);
  EXPECT_EQ(Mod->getFuncExportNum(), 6U);
  // A module without a backend would return RuntimeError.
  WasmEdge::Host::StableDiffusion::Guest G;
  EXPECT_EQ(G.call(*Mod, "session_open",
                   {G.put("{"sv), UINT32_C(1), G.put(std::string(4, '\0'))}),
            static_cast<uint32_t>(
                WasmEdge::Host::StableDiffusion::ErrNo::InvalidEncoding));
}

TEST_F(SDContractTest, OpenGenerateReadFeedBackAndClose) {
  const auto Id = session();
  ASSERT_EQ(generate(Id, R"({"prompt":"a lovely cat","width":256,"height":256,)"
                         R"("steps":4,"seed":42,"batch_count":2})"),
            E(Success));
  EXPECT_EQ(resultCount(), 2U);
  const auto Png = readResult(Id, 1, 0);
  const auto Head = WasmEdge::Host::StableDiffusion::probeImage(Png);
  ASSERT_TRUE(Head);
  EXPECT_EQ(Head->Width, 256U);
  EXPECT_EQ(Head->Height, 256U);
  EXPECT_EQ(Head->Channels, 3U);
  const auto Raw = readResult(Id, 0, 1);
  EXPECT_EQ(Raw.size(), 256U * 256U * 3U);
  EXPECT_TRUE(std::any_of(Raw.begin(), Raw.end(),
                          [](uint8_t Byte) { return Byte != 0; }));
  ASSERT_EQ(generate(Id,
                     R"({"prompt":"a lovely dog","width":256,"height":256,)"
                     R"("steps":4,"seed":42,"strength":0.7,)"
                     R"("init_image":{"blob":0}})",
                     {Png}),
            E(Success));
  EXPECT_EQ(resultCount(), 1U);
  EXPECT_EQ(close(Id), E(Success));
  EXPECT_EQ(generate(Id, small()), E(NotFound));
}

} // namespace
