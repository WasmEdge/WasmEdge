// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/defines.h"
#include "common/filesystem.h"
#include "opencvmini_module.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <gtest/gtest.h>
#include <initializer_list>
#include <memory>
#include <string_view>
#include <type_traits>

namespace {

template <typename T, typename U>
inline std::unique_ptr<T> dynamicPointerCast(std::unique_ptr<U> &&R) noexcept {
  static_assert(std::has_virtual_destructor_v<T>);
  T *P = dynamic_cast<T *>(R.get());
  if (P) {
    R.release();
  }
  return std::unique_ptr<T>(P);
}

std::unique_ptr<WasmEdge::Host::WasmEdgeOpenCVMiniModule> createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(WasmEdge::u8path(
      "../../../plugins/wasmedge_opencvmini/" WASMEDGE_LIB_PREFIX
      "wasmedgePluginWasmEdgeOpenCVMini" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_opencvmini"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_opencvmini"sv)) {
      return dynamicPointerCast<WasmEdge::Host::WasmEdgeOpenCVMiniModule>(
          Module->create());
    }
  }
  return {};
}

// Test image: #FF0000 png file, 30x30 image size, 158 bytes.
constexpr std::array<uint8_t, 158> TestRedPNG = {
    0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
    0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x1EU, 0x00U, 0x00U,
    0x00U, 0x1EU, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x3BU, 0x30U, 0xAEU, 0xA2U,
    0x00U, 0x00U, 0x00U, 0x01U, 0x73U, 0x52U, 0x47U, 0x42U, 0x00U, 0xAEU, 0xCEU,
    0x1CU, 0xE9U, 0x00U, 0x00U, 0x00U, 0x04U, 0x67U, 0x41U, 0x4DU, 0x41U, 0x00U,
    0x00U, 0xB1U, 0x8FU, 0x0BU, 0xFCU, 0x61U, 0x05U, 0x00U, 0x00U, 0x00U, 0x09U,
    0x70U, 0x48U, 0x59U, 0x73U, 0x00U, 0x00U, 0x16U, 0x25U, 0x00U, 0x00U, 0x16U,
    0x25U, 0x01U, 0x49U, 0x52U, 0x24U, 0xF0U, 0x00U, 0x00U, 0x00U, 0x33U, 0x49U,
    0x44U, 0x41U, 0x54U, 0x48U, 0x4BU, 0xEDU, 0xCDU, 0xA1U, 0x01U, 0x00U, 0x00U,
    0x0CU, 0x83U, 0xB0U, 0xFEU, 0xFFU, 0x74U, 0xE7U, 0x77U, 0x00U, 0x35U, 0x88U,
    0x18U, 0x0CU, 0x69U, 0xD2U, 0x85U, 0xFCU, 0x40U, 0x71U, 0x8CU, 0x71U, 0x8CU,
    0x71U, 0x8CU, 0x71U, 0x8CU, 0x71U, 0x8CU, 0x71U, 0x8CU, 0x71U, 0x8CU, 0x71U,
    0x8CU, 0x71U, 0x8CU, 0x99U, 0x8DU, 0x0FU, 0xD5U, 0x6CU, 0x01U, 0x62U, 0x5DU,
    0xE8U, 0xB5U, 0x3DU, 0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U,
    0xAEU, 0x42U, 0x60U, 0x82U};

} // namespace

TEST(WasmEdgeOpenCVMiniTest, Module) {
  // Create the wasmedge_opencvmini module instance.
  auto ImgMod = createModule();
  ASSERT_TRUE(ImgMod);
  EXPECT_EQ(ImgMod->getFuncExportNum(), 18U);
  EXPECT_NE(ImgMod->findFuncExports("wasmedge_opencvmini_imdecode"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("wasmedge_opencvmini_imencode"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("wasmedge_opencvmini_rectangle"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("wasmedge_opencvmini_cvt_color"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("wasmedge_opencvmini_release_mat"),
            nullptr);
  EXPECT_EQ(ImgMod->findFuncExports("wasmedge_opencvmini_imshow"), nullptr);
  EXPECT_EQ(ImgMod->findFuncExports("wasmedge_opencvmini_waitkey"), nullptr);
}

TEST(WasmEdgeOpenCVMiniTest, ExecuteHostFunction) {
  auto ImgMod = createModule();
  ASSERT_TRUE(ImgMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInst = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInst, nullptr);
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  const uint32_t InOffset = 0;
  const uint32_t ExtOffset = 512;
  const uint32_t OutOffset = 1024;
  const uint32_t OutCap = 32768;
  constexpr std::string_view Ext = ".png";
  std::copy(TestRedPNG.begin(), TestRedPNG.end(),
            MemInst->getPointer<uint8_t *>(InOffset));
  std::copy(Ext.begin(), Ext.end(), MemInst->getPointer<char *>(ExtOffset));

  // Decode the PNG payload into a matrix.
  auto *DecodeInst = ImgMod->findFuncExports("wasmedge_opencvmini_imdecode");
  ASSERT_NE(DecodeInst, nullptr);
  ASSERT_TRUE(DecodeInst->isHostFunction());
  std::array<WasmEdge::ValVariant, 1> DecodeRets = {UINT32_C(0)};
  ASSERT_TRUE(DecodeInst->getHostFunc().run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          InOffset, static_cast<uint32_t>(TestRedPNG.size())},
      DecodeRets));
  const uint32_t MatKey = DecodeRets[0].get<uint32_t>();

  // Encode the matrix back into guest memory.
  auto *EncodeInst = ImgMod->findFuncExports("wasmedge_opencvmini_imencode");
  ASSERT_NE(EncodeInst, nullptr);
  ASSERT_TRUE(EncodeInst->isHostFunction());
  ASSERT_TRUE(EncodeInst->getHostFunc().run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          ExtOffset, static_cast<uint32_t>(Ext.size()), MatKey, OutOffset,
          OutCap},
      {}));

  // The output buffer must now hold a PNG stream.
  constexpr std::array<uint8_t, 8> PngMagic = {0x89U, 0x50U, 0x4EU, 0x47U,
                                               0x0DU, 0x0AU, 0x1AU, 0x0AU};
  auto OutSpan = MemInst->getSpan<const uint8_t>(OutOffset, PngMagic.size());
  ASSERT_EQ(OutSpan.size(), PngMagic.size());
  EXPECT_TRUE(std::equal(PngMagic.begin(), PngMagic.end(), OutSpan.begin()));
}

TEST(WasmEdgeOpenCVMiniTest, MatrixHandles) {
  auto ImgMod = createModule();
  ASSERT_TRUE(ImgMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInst = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInst, nullptr);
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  const uint32_t InOffset = 0;
  const uint32_t ExtOffset = 512;
  const uint32_t OutOffset = 1024;
  const uint32_t OutCap = 32768;
  constexpr std::string_view Ext = ".png";
  std::copy(TestRedPNG.begin(), TestRedPNG.end(),
            MemInst->getPointer<uint8_t *>(InOffset));
  std::copy(Ext.begin(), Ext.end(), MemInst->getPointer<char *>(ExtOffset));

  auto Run = [&](std::string_view Name,
                 std::initializer_list<WasmEdge::ValVariant> Args,
                 WasmEdge::Span<WasmEdge::ValVariant> Rets) -> bool {
    auto *FuncInst = ImgMod->findFuncExports(Name);
    EXPECT_NE(FuncInst, nullptr);
    return FuncInst != nullptr &&
           FuncInst->getHostFunc().run(CallFrame, Args, Rets).has_value();
  };
  auto Decode = [&]() -> uint32_t {
    std::array<WasmEdge::ValVariant, 1> Rets = {UINT32_C(0)};
    EXPECT_TRUE(Run("wasmedge_opencvmini_imdecode",
                    {InOffset, static_cast<uint32_t>(TestRedPNG.size())},
                    Rets));
    return Rets[0].get<uint32_t>();
  };
  auto Encode = [&](uint32_t MatKey) {
    return Run("wasmedge_opencvmini_imencode",
               {ExtOffset, static_cast<uint32_t>(Ext.size()), MatKey, OutOffset,
                OutCap},
               {});
  };
  std::array<WasmEdge::ValVariant, 1> Rets = {UINT32_C(0)};

  // Two matrices of the same type and a filter result get distinct handles,
  // and the source stays usable after the filter.
  const uint32_t First = Decode();
  const uint32_t Second = Decode();
  EXPECT_NE(First, Second);
  ASSERT_TRUE(
      Run("wasmedge_opencvmini_blur", {First, UINT32_C(3), UINT32_C(3)}, Rets));
  const uint32_t Blurred = Rets[0].get<uint32_t>();
  EXPECT_NE(Blurred, First);
  EXPECT_NE(Blurred, Second);
  EXPECT_TRUE(Encode(First));
  EXPECT_TRUE(Encode(Second));
  EXPECT_TRUE(Encode(Blurred));

  // Unknown source and kernel handles are rejected.
  const uint32_t Unknown = UINT32_C(0xFFFFFFFF);
  EXPECT_FALSE(Run("wasmedge_opencvmini_blur",
                   {Unknown, UINT32_C(3), UINT32_C(3)}, Rets));
  EXPECT_FALSE(Run("wasmedge_opencvmin_dilate", {Unknown, First}, Rets));
  EXPECT_FALSE(Run("wasmedge_opencvmin_dilate", {First, Unknown}, Rets));
  EXPECT_FALSE(Run("wasmedge_opencvmini_erode", {Unknown, First}, Rets));
  EXPECT_FALSE(Run("wasmedge_opencvmini_erode", {First, Unknown}, Rets));

  // A released handle is rejected, and the other handles stay usable.
  EXPECT_TRUE(Run("wasmedge_opencvmini_release_mat", {First}, {}));
  EXPECT_FALSE(Encode(First));
  EXPECT_FALSE(Run("wasmedge_opencvmini_release_mat", {First}, {}));
  EXPECT_TRUE(Encode(Second));
  EXPECT_TRUE(Encode(Blurred));
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
