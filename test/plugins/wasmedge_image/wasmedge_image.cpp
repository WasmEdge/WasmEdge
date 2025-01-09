// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "image_func.h"
#include "image_module.h"
#include "runtime/instance/module.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

using WasmEdge::Host::WasmEdgeImage::ErrNo;

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

std::unique_ptr<WasmEdge::Host::WasmEdgeImageModule> createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_image/" WASMEDGE_LIB_PREFIX
      "wasmedgePluginWasmEdgeImage" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasmedge_image"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_image"sv)) {
      return dynamicPointerCast<WasmEdge::Host::WasmEdgeImageModule>(
          Module->create());
    }
  }
  return {};
}

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset, uint32_t Cnt, uint8_t C = 0) noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(Offset), Cnt, C);
}

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset,
                    const std::vector<uint8_t> &Payload) noexcept {
  uint8_t *Buf = MemInst.getPointer<uint8_t *>(Offset);
  std::copy_n(Payload.data(), Payload.size(), Buf);
}

// Test image: #FF0000 png file, 30x30 image size, 158 bytes.
std::vector<uint8_t> TestRedPNG = {
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

// Test image: #FF0000 jpg file, 30x30 image size, 647 bytes.
std::vector<uint8_t> TestRedJPG = {
    0xFFU, 0xD8U, 0xFFU, 0xE0U, 0x00U, 0x10U, 0x4AU, 0x46U, 0x49U, 0x46U, 0x00U,
    0x01U, 0x01U, 0x01U, 0x00U, 0x90U, 0x00U, 0x90U, 0x00U, 0x00U, 0xFFU, 0xDBU,
    0x00U, 0x43U, 0x00U, 0x02U, 0x01U, 0x01U, 0x02U, 0x01U, 0x01U, 0x02U, 0x02U,
    0x02U, 0x02U, 0x02U, 0x02U, 0x02U, 0x02U, 0x03U, 0x05U, 0x03U, 0x03U, 0x03U,
    0x03U, 0x03U, 0x06U, 0x04U, 0x04U, 0x03U, 0x05U, 0x07U, 0x06U, 0x07U, 0x07U,
    0x07U, 0x06U, 0x07U, 0x07U, 0x08U, 0x09U, 0x0BU, 0x09U, 0x08U, 0x08U, 0x0AU,
    0x08U, 0x07U, 0x07U, 0x0AU, 0x0DU, 0x0AU, 0x0AU, 0x0BU, 0x0CU, 0x0CU, 0x0CU,
    0x0CU, 0x07U, 0x09U, 0x0EU, 0x0FU, 0x0DU, 0x0CU, 0x0EU, 0x0BU, 0x0CU, 0x0CU,
    0x0CU, 0xFFU, 0xDBU, 0x00U, 0x43U, 0x01U, 0x02U, 0x02U, 0x02U, 0x03U, 0x03U,
    0x03U, 0x06U, 0x03U, 0x03U, 0x06U, 0x0CU, 0x08U, 0x07U, 0x08U, 0x0CU, 0x0CU,
    0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU,
    0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU,
    0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU,
    0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU, 0x0CU,
    0x0CU, 0x0CU, 0x0CU, 0x0CU, 0xFFU, 0xC0U, 0x00U, 0x11U, 0x08U, 0x00U, 0x1EU,
    0x00U, 0x1EU, 0x03U, 0x01U, 0x22U, 0x00U, 0x02U, 0x11U, 0x01U, 0x03U, 0x11U,
    0x01U, 0xFFU, 0xC4U, 0x00U, 0x1FU, 0x00U, 0x00U, 0x01U, 0x05U, 0x01U, 0x01U,
    0x01U, 0x01U, 0x01U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U, 0x09U, 0x0AU,
    0x0BU, 0xFFU, 0xC4U, 0x00U, 0xB5U, 0x10U, 0x00U, 0x02U, 0x01U, 0x03U, 0x03U,
    0x02U, 0x04U, 0x03U, 0x05U, 0x05U, 0x04U, 0x04U, 0x00U, 0x00U, 0x01U, 0x7DU,
    0x01U, 0x02U, 0x03U, 0x00U, 0x04U, 0x11U, 0x05U, 0x12U, 0x21U, 0x31U, 0x41U,
    0x06U, 0x13U, 0x51U, 0x61U, 0x07U, 0x22U, 0x71U, 0x14U, 0x32U, 0x81U, 0x91U,
    0xA1U, 0x08U, 0x23U, 0x42U, 0xB1U, 0xC1U, 0x15U, 0x52U, 0xD1U, 0xF0U, 0x24U,
    0x33U, 0x62U, 0x72U, 0x82U, 0x09U, 0x0AU, 0x16U, 0x17U, 0x18U, 0x19U, 0x1AU,
    0x25U, 0x26U, 0x27U, 0x28U, 0x29U, 0x2AU, 0x34U, 0x35U, 0x36U, 0x37U, 0x38U,
    0x39U, 0x3AU, 0x43U, 0x44U, 0x45U, 0x46U, 0x47U, 0x48U, 0x49U, 0x4AU, 0x53U,
    0x54U, 0x55U, 0x56U, 0x57U, 0x58U, 0x59U, 0x5AU, 0x63U, 0x64U, 0x65U, 0x66U,
    0x67U, 0x68U, 0x69U, 0x6AU, 0x73U, 0x74U, 0x75U, 0x76U, 0x77U, 0x78U, 0x79U,
    0x7AU, 0x83U, 0x84U, 0x85U, 0x86U, 0x87U, 0x88U, 0x89U, 0x8AU, 0x92U, 0x93U,
    0x94U, 0x95U, 0x96U, 0x97U, 0x98U, 0x99U, 0x9AU, 0xA2U, 0xA3U, 0xA4U, 0xA5U,
    0xA6U, 0xA7U, 0xA8U, 0xA9U, 0xAAU, 0xB2U, 0xB3U, 0xB4U, 0xB5U, 0xB6U, 0xB7U,
    0xB8U, 0xB9U, 0xBAU, 0xC2U, 0xC3U, 0xC4U, 0xC5U, 0xC6U, 0xC7U, 0xC8U, 0xC9U,
    0xCAU, 0xD2U, 0xD3U, 0xD4U, 0xD5U, 0xD6U, 0xD7U, 0xD8U, 0xD9U, 0xDAU, 0xE1U,
    0xE2U, 0xE3U, 0xE4U, 0xE5U, 0xE6U, 0xE7U, 0xE8U, 0xE9U, 0xEAU, 0xF1U, 0xF2U,
    0xF3U, 0xF4U, 0xF5U, 0xF6U, 0xF7U, 0xF8U, 0xF9U, 0xFAU, 0xFFU, 0xC4U, 0x00U,
    0x1FU, 0x01U, 0x00U, 0x03U, 0x01U, 0x01U, 0x01U, 0x01U, 0x01U, 0x01U, 0x01U,
    0x01U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x02U, 0x03U,
    0x04U, 0x05U, 0x06U, 0x07U, 0x08U, 0x09U, 0x0AU, 0x0BU, 0xFFU, 0xC4U, 0x00U,
    0xB5U, 0x11U, 0x00U, 0x02U, 0x01U, 0x02U, 0x04U, 0x04U, 0x03U, 0x04U, 0x07U,
    0x05U, 0x04U, 0x04U, 0x00U, 0x01U, 0x02U, 0x77U, 0x00U, 0x01U, 0x02U, 0x03U,
    0x11U, 0x04U, 0x05U, 0x21U, 0x31U, 0x06U, 0x12U, 0x41U, 0x51U, 0x07U, 0x61U,
    0x71U, 0x13U, 0x22U, 0x32U, 0x81U, 0x08U, 0x14U, 0x42U, 0x91U, 0xA1U, 0xB1U,
    0xC1U, 0x09U, 0x23U, 0x33U, 0x52U, 0xF0U, 0x15U, 0x62U, 0x72U, 0xD1U, 0x0AU,
    0x16U, 0x24U, 0x34U, 0xE1U, 0x25U, 0xF1U, 0x17U, 0x18U, 0x19U, 0x1AU, 0x26U,
    0x27U, 0x28U, 0x29U, 0x2AU, 0x35U, 0x36U, 0x37U, 0x38U, 0x39U, 0x3AU, 0x43U,
    0x44U, 0x45U, 0x46U, 0x47U, 0x48U, 0x49U, 0x4AU, 0x53U, 0x54U, 0x55U, 0x56U,
    0x57U, 0x58U, 0x59U, 0x5AU, 0x63U, 0x64U, 0x65U, 0x66U, 0x67U, 0x68U, 0x69U,
    0x6AU, 0x73U, 0x74U, 0x75U, 0x76U, 0x77U, 0x78U, 0x79U, 0x7AU, 0x82U, 0x83U,
    0x84U, 0x85U, 0x86U, 0x87U, 0x88U, 0x89U, 0x8AU, 0x92U, 0x93U, 0x94U, 0x95U,
    0x96U, 0x97U, 0x98U, 0x99U, 0x9AU, 0xA2U, 0xA3U, 0xA4U, 0xA5U, 0xA6U, 0xA7U,
    0xA8U, 0xA9U, 0xAAU, 0xB2U, 0xB3U, 0xB4U, 0xB5U, 0xB6U, 0xB7U, 0xB8U, 0xB9U,
    0xBAU, 0xC2U, 0xC3U, 0xC4U, 0xC5U, 0xC6U, 0xC7U, 0xC8U, 0xC9U, 0xCAU, 0xD2U,
    0xD3U, 0xD4U, 0xD5U, 0xD6U, 0xD7U, 0xD8U, 0xD9U, 0xDAU, 0xE2U, 0xE3U, 0xE4U,
    0xE5U, 0xE6U, 0xE7U, 0xE8U, 0xE9U, 0xEAU, 0xF2U, 0xF3U, 0xF4U, 0xF5U, 0xF6U,
    0xF7U, 0xF8U, 0xF9U, 0xFAU, 0xFFU, 0xDAU, 0x00U, 0x0CU, 0x03U, 0x01U, 0x00U,
    0x02U, 0x11U, 0x03U, 0x11U, 0x00U, 0x3FU, 0x00U, 0xF8U, 0xBEU, 0x8AU, 0x28U,
    0xAFU, 0xE5U, 0x33U, 0xFDU, 0xFCU, 0x0AU, 0x28U, 0xA2U, 0x80U, 0x0AU, 0x28U,
    0xA2U, 0x80U, 0x0AU, 0x28U, 0xA2U, 0x80U, 0x3FU, 0xFFU, 0xD9U};

} // namespace

TEST(WasmEdgeImageTest, Module) {
  // Create the wasmedge_image module instance.
  auto ImgMod = createModule();
  ASSERT_TRUE(ImgMod);
  EXPECT_EQ(ImgMod->getFuncExportNum(), 3U);
  EXPECT_NE(ImgMod->findFuncExports("load_jpg"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("load_png"), nullptr);
  EXPECT_NE(ImgMod->findFuncExports("load_image"), nullptr);
}

TEST(WasmEdgeImageTest, LoadJPG) {
  // Create the wasmedge_image module instance.
  auto ImgMod = createModule();
  ASSERT_TRUE(ImgMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Set the target size.
  uint32_t TargetW = 50, TargetH = 60;
  uint32_t TargetSize = TargetW * TargetH * 3;
  // Assume the pixel position (45, 55).
  uint32_t Position = 55 * TargetW + 45;
  // Input payload offset.
  uint32_t InOffset = 0;
  // Output image data offset.
  uint32_t OutOffset = 1024;
  // Output image span.
  WasmEdge::Span<const uint8_t> OutSpanU8;
  WasmEdge::Span<const float> OutSpanF32;

  // Get the function "load_jpg".
  auto *FuncInst = ImgMod->findFuncExports("load_jpg");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst = dynamic_cast<WasmEdge::Host::WasmEdgeImage::LoadJPG &>(
      FuncInst->getHostFunc());

  // Test: Load JPG and resize into 50x60 RGB u8 format.
  // Clear the memory[0, 32768].
  fillMemContent(MemInst, 0, 32768);
  // Set the memory[0, 647] as the JPG image payload.
  fillMemContent(MemInst, 0, TestRedJPG);
  // Get output image span.
  OutSpanU8 = MemInst.getSpan<const uint8_t>(OutOffset, TargetSize);
  // Run.
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          InOffset,                                 // Payload offset.
          static_cast<uint32_t>(TestRedJPG.size()), // Payload size.
          TargetW, TargetH,                         // Target width and height.
          0U,                                       // Target type: RGB8.
          OutOffset,                                // Output buffer offset.
          TargetSize *
              static_cast<uint32_t>(sizeof(uint8_t)) // Output buffer size.
      },
      Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), static_cast<uint32_t>(ErrNo::Success));
  // Note: Due to the JPG compression, the R is 254, not 255 here.
  EXPECT_EQ(OutSpanU8[Position * 3], UINT8_C(254));
  EXPECT_EQ(OutSpanU8[Position * 3 + 1], UINT8_C(0));
  EXPECT_EQ(OutSpanU8[Position * 3 + 2], UINT8_C(0));

  // Test: Load JPG and resize into 50x60 BGR u8 format.
  // Clear the memory[0, 32768].
  fillMemContent(MemInst, 0, 32768);
  // Set the memory[0, 158] as the JPG image payload.
  fillMemContent(MemInst, 0, TestRedJPG);
  // Get output image span.
  OutSpanU8 = MemInst.getSpan<uint8_t>(OutOffset, TargetSize);
  // Run.
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          InOffset,                                 // Payload offset.
          static_cast<uint32_t>(TestRedJPG.size()), // Payload size.
          TargetW, TargetH,                         // Target width and height.
          1U,                                       // Target type: BGR8.
          OutOffset,                                // Output buffer offset.
          TargetSize *
              static_cast<uint32_t>(sizeof(uint8_t)) // Output buffer size.
      },
      Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), static_cast<uint32_t>(ErrNo::Success));
  // Note: Due to the JPG compression, the R is 254, not 255 here.
  EXPECT_EQ(OutSpanU8[Position * 3], UINT8_C(0));
  EXPECT_EQ(OutSpanU8[Position * 3 + 1], UINT8_C(0));
  EXPECT_EQ(OutSpanU8[Position * 3 + 2], UINT8_C(254));

  // Test: Load JPG and resize into 50x60 RGB f32 format.
  // Clear the memory[0, 32768].
  fillMemContent(MemInst, 0, 32768);
  // Set the memory[0, 647] as the JPG image payload.
  fillMemContent(MemInst, 0, TestRedJPG);
  // Get output image span.
  OutSpanF32 = MemInst.getSpan<float>(OutOffset, TargetSize);
  // Run.
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          InOffset,                                 // Payload offset.
          static_cast<uint32_t>(TestRedJPG.size()), // Payload size.
          TargetW, TargetH,                         // Target width and height.
          2U,                                       // Target type: RGB32F.
          OutOffset,                                // Output buffer offset.
          TargetSize *
              static_cast<uint32_t>(sizeof(float)) // Output buffer size.
      },
      Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), static_cast<uint32_t>(ErrNo::Success));
  // Note: Due to the JPG compression, the R is 0.991392851f, not 1.0f here.
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3] - 1.0f) < 0.01f);
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3 + 1] - 0.0f) < 0.01f);
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3 + 2] - 0.0f) < 0.01f);

  // Test: Load JPG and resize into 50x60 BGR f32 format.
  // Clear the memory[0, 32768].
  fillMemContent(MemInst, 0, 32768);
  // Set the memory[0, 647] as the JPG image payload.
  fillMemContent(MemInst, 0, TestRedJPG);
  // Get output image span.
  OutSpanF32 = MemInst.getSpan<float>(OutOffset, TargetSize);
  // Run.
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          InOffset,                                 // Payload offset.
          static_cast<uint32_t>(TestRedJPG.size()), // Payload size.
          TargetW, TargetH,                         // Target width and height.
          3U,                                       // Target type: BGR32F.
          OutOffset,                                // Output buffer offset.
          TargetSize *
              static_cast<uint32_t>(sizeof(float)) // Output buffer size.
      },
      Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), static_cast<uint32_t>(ErrNo::Success));
  // Note: Due to the JPG compression, the R is 0.991392851f, not 1.0f here.
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3] - 0.0f) < 0.01f);
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3 + 1] - 0.0f) < 0.01f);
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3 + 2] - 1.0f) < 0.01f);
}

TEST(WasmEdgeImageTest, LoadPNG) {
  // Create the wasmedge_image module instance.
  auto ImgMod = createModule();
  ASSERT_TRUE(ImgMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Set the target size.
  uint32_t TargetW = 50, TargetH = 60;
  uint32_t TargetSize = TargetW * TargetH * 3;
  // Assume the pixel position (45, 55).
  uint32_t Position = 55 * TargetW + 45;
  // Input payload offset.
  uint32_t InOffset = 0;
  // Output image data offset.
  uint32_t OutOffset = 1024;
  // Output image span.
  WasmEdge::Span<const uint8_t> OutSpanU8;
  WasmEdge::Span<const float> OutSpanF32;

  // Get the function "load_png".
  auto *FuncInst = ImgMod->findFuncExports("load_png");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst = dynamic_cast<WasmEdge::Host::WasmEdgeImage::LoadPNG &>(
      FuncInst->getHostFunc());

  // Test: Load PNG and resize into 50x60 RGB u8 format.
  // Clear the memory[0, 32768].
  fillMemContent(MemInst, 0, 32768);
  // Set the memory[0, 158] as the PNG image payload.
  fillMemContent(MemInst, 0, TestRedPNG);
  // Get output image span.
  OutSpanU8 = MemInst.getSpan<const uint8_t>(OutOffset, TargetSize);
  // Run.
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          InOffset,                                 // Payload offset.
          static_cast<uint32_t>(TestRedPNG.size()), // Payload size.
          TargetW, TargetH,                         // Target width and height.
          0U,                                       // Target type: RGB8.
          OutOffset,                                // Output buffer offset.
          TargetSize *
              static_cast<uint32_t>(sizeof(uint8_t)) // Output buffer size.
      },
      Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), static_cast<uint32_t>(ErrNo::Success));
  EXPECT_EQ(OutSpanU8[Position * 3], UINT8_C(255));
  EXPECT_EQ(OutSpanU8[Position * 3 + 1], UINT8_C(0));
  EXPECT_EQ(OutSpanU8[Position * 3 + 2], UINT8_C(0));

  // Test: Load PNG and resize into 50x60 BGR u8 format.
  // Clear the memory[0, 32768].
  fillMemContent(MemInst, 0, 32768);
  // Set the memory[0, 158] as the PNG image payload.
  fillMemContent(MemInst, 0, TestRedPNG);
  // Get output image span.
  OutSpanU8 = MemInst.getSpan<uint8_t>(OutOffset, TargetSize);
  // Run.
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          InOffset,                                 // Payload offset.
          static_cast<uint32_t>(TestRedPNG.size()), // Payload size.
          TargetW, TargetH,                         // Target width and height.
          1U,                                       // Target type: BGR8.
          OutOffset,                                // Output buffer offset.
          TargetSize *
              static_cast<uint32_t>(sizeof(uint8_t)) // Output buffer size.
      },
      Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), static_cast<uint32_t>(ErrNo::Success));
  EXPECT_EQ(OutSpanU8[Position * 3], UINT8_C(0));
  EXPECT_EQ(OutSpanU8[Position * 3 + 1], UINT8_C(0));
  EXPECT_EQ(OutSpanU8[Position * 3 + 2], UINT8_C(255));

  // Test: Load PNG and resize into 50x60 RGB f32 format.
  // Clear the memory[0, 32768].
  fillMemContent(MemInst, 0, 32768);
  // Set the memory[0, 158] as the PNG image payload.
  fillMemContent(MemInst, 0, TestRedPNG);
  // Get output image span.
  OutSpanF32 = MemInst.getSpan<float>(OutOffset, TargetSize);
  // Run.
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          InOffset,                                 // Payload offset.
          static_cast<uint32_t>(TestRedPNG.size()), // Payload size.
          TargetW, TargetH,                         // Target width and height.
          2U,                                       // Target type: RGB32F.
          OutOffset,                                // Output buffer offset.
          TargetSize *
              static_cast<uint32_t>(sizeof(float)) // Output buffer size.
      },
      Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), static_cast<uint32_t>(ErrNo::Success));
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3] - 1.0f) < 0.00001f);
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3 + 1] - 0.0f) < 0.00001f);
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3 + 2] - 0.0f) < 0.00001f);

  // Test: Load PNG and resize into 50x60 BGR f32 format.
  // Clear the memory[0, 32768].
  fillMemContent(MemInst, 0, 32768);
  // Set the memory[0, 158] as the PNG image payload.
  fillMemContent(MemInst, 0, TestRedPNG);
  // Get output image span.
  OutSpanF32 = MemInst.getSpan<float>(OutOffset, TargetSize);
  // Run.
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          InOffset,                                 // Payload offset.
          static_cast<uint32_t>(TestRedPNG.size()), // Payload size.
          TargetW, TargetH,                         // Target width and height.
          3U,                                       // Target type: BGR32F.
          OutOffset,                                // Output buffer offset.
          TargetSize *
              static_cast<uint32_t>(sizeof(float)) // Output buffer size.
      },
      Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), static_cast<uint32_t>(ErrNo::Success));
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3] - 0.0f) < 0.00001f);
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3 + 1] - 0.0f) < 0.00001f);
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3 + 2] - 1.0f) < 0.00001f);
}

TEST(WasmEdgeImageTest, LoadImage) {
  // Test for the general API.

  // Create the wasmedge_image module instance.
  auto ImgMod = createModule();
  ASSERT_TRUE(ImgMod);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  // Set the target size.
  uint32_t TargetW = 50, TargetH = 60;
  uint32_t TargetSize = TargetW * TargetH * 3;
  // Assume the pixel position (45, 55).
  uint32_t Position = 55 * TargetW + 45;
  // Input payload offset.
  uint32_t InOffset = 0;
  // Output image data offset.
  uint32_t OutOffset = 1024;
  // Output image span.
  WasmEdge::Span<const uint8_t> OutSpanU8;
  WasmEdge::Span<const float> OutSpanF32;

  // Get the function "load_image".
  auto *FuncInst = ImgMod->findFuncExports("load_image");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst = dynamic_cast<WasmEdge::Host::WasmEdgeImage::LoadImage &>(
      FuncInst->getHostFunc());

  // Test: Load JPG and resize into 50x60 BGR u8 format.
  // Clear the memory[0, 32768].
  fillMemContent(MemInst, 0, 32768);
  // Set the memory[0, 647] as the JPG image payload.
  fillMemContent(MemInst, 0, TestRedJPG);
  // Get output image span.
  OutSpanU8 = MemInst.getSpan<uint8_t>(OutOffset, TargetSize);
  // Run.
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          InOffset,                                 // Payload offset.
          static_cast<uint32_t>(TestRedJPG.size()), // Payload size.
          TargetW, TargetH,                         // Target width and height.
          1U,                                       // Target type: BGR8.
          OutOffset,                                // Output buffer offset.
          TargetSize *
              static_cast<uint32_t>(sizeof(uint8_t)) // Output buffer size.
      },
      Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), static_cast<uint32_t>(ErrNo::Success));
  // Note: Due to the JPG compression, the R is 254, not 255 here.
  EXPECT_EQ(OutSpanU8[Position * 3], UINT8_C(0));
  EXPECT_EQ(OutSpanU8[Position * 3 + 1], UINT8_C(0));
  EXPECT_EQ(OutSpanU8[Position * 3 + 2], UINT8_C(254));

  // Test: Load PNG and resize into 50x60 RGB f32 format.
  // Clear the memory[0, 32768].
  fillMemContent(MemInst, 0, 32768);
  // Set the memory[0, 647] as the PNG image payload.
  fillMemContent(MemInst, 0, TestRedPNG);
  // Get output image span.
  OutSpanF32 = MemInst.getSpan<float>(OutOffset, TargetSize);
  // Run.
  EXPECT_TRUE(HostFuncInst.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          InOffset,                                 // Payload offset.
          static_cast<uint32_t>(TestRedPNG.size()), // Payload size.
          TargetW, TargetH,                         // Target width and height.
          2U,                                       // Target type: RGB32F.
          OutOffset,                                // Output buffer offset.
          TargetSize *
              static_cast<uint32_t>(sizeof(float)) // Output buffer size.
      },
      Errno));
  EXPECT_EQ(Errno[0].get<uint32_t>(), static_cast<uint32_t>(ErrNo::Success));
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3] - 1.0f) < 0.00001f);
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3 + 1] - 0.0f) < 0.00001f);
  EXPECT_TRUE(std::fabs(OutSpanF32[Position * 3 + 2] - 0.0f) < 0.00001f);
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
