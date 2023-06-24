// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#include "runtime/instance/module.h"
#include "zlibfunc.h"
#include "zlibmodule.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {
WasmEdge::Runtime::CallingFrame DummyCallFrame(nullptr, nullptr);

WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_zlib/"
      "libwasmedgePluginWasmEdgeZlib" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasmedge_zlib"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_zlib"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}

} // namespace

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset, uint32_t Cnt, uint8_t C = 0) noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(Offset), Cnt, C);
}

static constexpr size_t DATA_SIZE = 1 * 1024 * 1024ULL;
// static constexpr size_t INPUT_BUFFER_SIZE = 32 * 1024llu;
static constexpr size_t OUTPUT_BUFFER_SIZE = 64 * 1024ULL;

constexpr auto randChar = []() -> char {
  constexpr char charset[] = "0123456789"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "abcdefghijklmnopqrstuvwxyz";
  constexpr size_t max_index = (sizeof(charset) - 1);
  return charset[std::rand() % max_index];
};

TEST(WasmEdgeZlibTest, DeflateInflateCycle) {
  auto *ZlibMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeZlibModule *>(createModule());
  ASSERT_TRUE(ZlibMod != nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16 * 64, 16 * 64)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  uint32_t
      // WASM Memory Heap Pointer
      wasm_hp = 0,
      wasm_data, wasm_zlib_version, wasm_z_stream, wasm_compressed_data,
      wasm_decompressed_data;
  uint32_t wasm_compressed_data_size = 0, wasm_decompressed_data_size = 0;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  auto *FuncInst = ZlibMod->findFuncExports("deflateInit_");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &__deflateInit_ =
      dynamic_cast<WasmEdge::Host::WasmEdgeZlibDeflateInit_ &>(
          FuncInst->getHostFunc());

  FuncInst = ZlibMod->findFuncExports("deflate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &__deflate = dynamic_cast<WasmEdge::Host::WasmEdgeZlibDeflate &>(
      FuncInst->getHostFunc());

  FuncInst = ZlibMod->findFuncExports("deflateEnd");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &__deflateEnd = dynamic_cast<WasmEdge::Host::WasmEdgeZlibDeflateEnd &>(
      FuncInst->getHostFunc());

  FuncInst = ZlibMod->findFuncExports("inflateInit_");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &__inflateInit_ =
      dynamic_cast<WasmEdge::Host::WasmEdgeZlibInflateInit_ &>(
          FuncInst->getHostFunc());

  FuncInst = ZlibMod->findFuncExports("inflate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &__inflate = dynamic_cast<WasmEdge::Host::WasmEdgeZlibInflate &>(
      FuncInst->getHostFunc());

  FuncInst = ZlibMod->findFuncExports("inflateEnd");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &__inflateEnd = dynamic_cast<WasmEdge::Host::WasmEdgeZlibInflateEnd &>(
      FuncInst->getHostFunc());

  std::array<WasmEdge::ValVariant, 1> RetVal;

  wasm_zlib_version = wasm_hp;
  std::snprintf(MemInst.getPointer<char *>(wasm_hp), std::strlen(ZLIB_VERSION),
                ZLIB_VERSION);
  wasm_hp += std::strlen(ZLIB_VERSION);

  wasm_data = wasm_hp;
  std::generate_n(MemInst.getPointer<char *>(wasm_hp), DATA_SIZE, randChar);
  wasm_hp += DATA_SIZE;

  wasm_z_stream = wasm_hp;
  Wasm_z_stream *strm = MemInst.getPointer<Wasm_z_stream *>(wasm_z_stream);
  wasm_hp += sizeof(Wasm_z_stream);

  // ----- Deflate Routine START------
  fillMemContent(MemInst, wasm_z_stream, sizeof(Wasm_z_stream), 0U);
  strm->zalloc = Z_NULL, strm->zfree = Z_NULL, strm->opaque = Z_NULL;

  // deflateInit_ Test
  // WASM z_stream size Mismatch
  EXPECT_FALSE(__deflateInit_.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{wasm_z_stream, INT32_C(-1),
                                                  wasm_zlib_version,
                                                  sizeof(Wasm_z_stream) + 16},
      RetVal));

  // Version Mismatch
  EXPECT_FALSE(__deflateInit_.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{wasm_z_stream, INT32_C(-1),
                                                  wasm_zlib_version + 2,
                                                  sizeof(Wasm_z_stream)},
      RetVal));

  EXPECT_TRUE(__deflateInit_.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          wasm_z_stream, INT32_C(-1), wasm_zlib_version, sizeof(Wasm_z_stream)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  wasm_compressed_data = wasm_hp;

  strm->avail_in = DATA_SIZE;
  strm->next_in = wasm_data;
  strm->avail_out = OUTPUT_BUFFER_SIZE;
  strm->next_out = wasm_compressed_data;

  // deflate Test
  do {
    if (strm->avail_out == 0) {
      wasm_hp += OUTPUT_BUFFER_SIZE;
      strm->avail_out = OUTPUT_BUFFER_SIZE;
      strm->next_out = wasm_hp;
    }

    EXPECT_TRUE(__deflate.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  wasm_z_stream,
                                  INT32_C(Z_FINISH),
                              },
                              RetVal));
    EXPECT_NE(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  } while (RetVal[0].get<int32_t>() != Z_STREAM_END);

  // deflateEnd Test
  EXPECT_TRUE(__deflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{wasm_z_stream},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  wasm_hp += OUTPUT_BUFFER_SIZE - strm->avail_out;
  wasm_compressed_data_size = wasm_hp - wasm_compressed_data;
  // ----- Deflate Routine END------

  // ----- Inflate Routine START------
  fillMemContent(MemInst, wasm_z_stream, sizeof(Wasm_z_stream), 0U);
  strm->zalloc = Z_NULL, strm->zfree = Z_NULL, strm->opaque = Z_NULL;

  // inflateInit_ Test
  // WASM z_stream size Mismatch
  EXPECT_FALSE(__inflateInit_.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          wasm_z_stream, wasm_zlib_version, sizeof(Wasm_z_stream) + 16},
      RetVal));

  // Version Mismatch
  EXPECT_FALSE(__inflateInit_.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          wasm_z_stream, wasm_zlib_version + 2, sizeof(Wasm_z_stream)},
      RetVal));

  EXPECT_TRUE(__inflateInit_.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          wasm_z_stream, wasm_zlib_version, sizeof(Wasm_z_stream)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  wasm_decompressed_data = wasm_hp;

  strm->avail_in = wasm_compressed_data_size;
  strm->next_in = wasm_compressed_data;
  strm->avail_out = OUTPUT_BUFFER_SIZE;
  strm->next_out = wasm_decompressed_data;

  // inflate test
  do {
    if (strm->avail_out == 0) {
      wasm_hp += OUTPUT_BUFFER_SIZE;
      strm->avail_out = OUTPUT_BUFFER_SIZE;
      strm->next_out = wasm_hp;
    }

    EXPECT_TRUE(__inflate.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  wasm_z_stream,
                                  INT32_C(Z_FINISH),
                              },
                              RetVal));
    EXPECT_NE(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  } while (RetVal[0].get<int32_t>() != Z_STREAM_END);

  EXPECT_TRUE(__inflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{wasm_z_stream},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  wasm_hp += OUTPUT_BUFFER_SIZE - strm->avail_out;
  wasm_decompressed_data_size = wasm_hp - wasm_decompressed_data;
  // ----- Inflate Routine END------

  // Test Decompressed Buffer size against source Data size.
  EXPECT_EQ(wasm_decompressed_data_size, DATA_SIZE);
  // Test Decompressed Buffer content against source Data.
  EXPECT_TRUE(
      std::equal(MemInst.getPointer<uint8_t *>(wasm_decompressed_data),
                 MemInst.getPointer<uint8_t *>(wasm_decompressed_data +
                                               wasm_decompressed_data_size),
                 MemInst.getPointer<uint8_t *>(wasm_data)));
}

TEST(WasmEdgeZlibTest, Module) {
  // Create the wasmedge_process module instance.
  auto *ZlibMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeZlibModule *>(createModule());
  EXPECT_FALSE(ZlibMod == nullptr);
  EXPECT_TRUE(ZlibMod->getEnv().ZStreamMap.empty());
  EXPECT_EQ(ZlibMod->getFuncExportNum(), 6U);
  EXPECT_NE(ZlibMod->findFuncExports("deflateInit_"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateInit_"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflate"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflate"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflateEnd"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateEnd"), nullptr);

  delete ZlibMod;
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
