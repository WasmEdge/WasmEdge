// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "runtime/instance/module.h"
#include "zlibfunc.h"
#include "zlibmodule.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace {

WasmEdge::Runtime::CallingFrame DummyCallFrame(nullptr, nullptr);

template <typename T, typename U>
inline std::unique_ptr<T> dynamicPointerCast(std::unique_ptr<U> &&R) noexcept {
  static_assert(std::has_virtual_destructor_v<T>);
  T *P = dynamic_cast<T *>(R.get());
  if (P) {
    R.release();
  }
  return std::unique_ptr<T>(P);
}

std::unique_ptr<WasmEdge::Host::WasmEdgeZlibModule> createModule() {
  using namespace std::literals::string_view_literals;
  WasmEdge::Plugin::Plugin::load(std::filesystem::u8path(
      "../../../plugins/wasmedge_zlib/" WASMEDGE_LIB_PREFIX
      "wasmedgePluginWasmEdgeZlib" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasmedge_zlib"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_zlib"sv)) {
      return dynamicPointerCast<WasmEdge::Host::WasmEdgeZlibModule>(
          Module->create());
    }
  }
  return {};
}

} // namespace

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset, uint32_t Cnt, uint8_t C = 0) noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(Offset), Cnt, C);
}

static constexpr size_t DataSize = 1 * 1024 * 1024ULL;
static constexpr size_t OutputBufferSize = 64 * 1024ULL;

constexpr auto RandChar = []() -> char {
  constexpr char Charset[] = "0123456789"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "abcdefghijklmnopqrstuvwxyz";
  constexpr size_t MaxIndex = (sizeof(Charset) - 1);
  return Charset[std::rand() % MaxIndex];
};

TEST(WasmEdgeZlibTest, DeflateInflateCycle) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

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
      WasmHP = 1,
      WasmData, WasmZlibVersion, ModuleZStream, WasmCompressedData,
      WasmDecompressedData;
  uint32_t WasmCompressedData_size = 0, WasmDecompressedDataSize = 0;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  auto *FuncInst = ZlibMod->findFuncExports("deflateInit_");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &DeflateInit_ = dynamic_cast<WasmEdge::Host::WasmEdgeZlibDeflateInit_ &>(
      FuncInst->getHostFunc());

  FuncInst = ZlibMod->findFuncExports("deflate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &Deflate = dynamic_cast<WasmEdge::Host::WasmEdgeZlibDeflate &>(
      FuncInst->getHostFunc());

  FuncInst = ZlibMod->findFuncExports("deflateEnd");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &DeflateEnd = dynamic_cast<WasmEdge::Host::WasmEdgeZlibDeflateEnd &>(
      FuncInst->getHostFunc());

  FuncInst = ZlibMod->findFuncExports("inflateInit_");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &InflateInit_ = dynamic_cast<WasmEdge::Host::WasmEdgeZlibInflateInit_ &>(
      FuncInst->getHostFunc());

  FuncInst = ZlibMod->findFuncExports("inflate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &Inflate = dynamic_cast<WasmEdge::Host::WasmEdgeZlibInflate &>(
      FuncInst->getHostFunc());

  FuncInst = ZlibMod->findFuncExports("inflateEnd");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &InflateEnd = dynamic_cast<WasmEdge::Host::WasmEdgeZlibInflateEnd &>(
      FuncInst->getHostFunc());

  std::array<WasmEdge::ValVariant, 1> RetVal;

  WasmZlibVersion = WasmHP;
  std::snprintf(MemInst.getPointer<char *>(WasmHP), std::strlen(ZLIB_VERSION),
                ZLIB_VERSION);
  WasmHP += std::strlen(ZLIB_VERSION);

  WasmData = WasmHP;
  std::generate_n(MemInst.getPointer<char *>(WasmHP), DataSize, RandChar);
  WasmHP += DataSize;

  ModuleZStream = WasmHP;
  WasmZStream *strm = MemInst.getPointer<WasmZStream *>(ModuleZStream);
  WasmHP += sizeof(WasmZStream);

  // ----- Deflate Routine START------
  fillMemContent(MemInst, ModuleZStream, sizeof(WasmZStream), 0U);

  // deflateInit_ Test
  // WASM z_stream size Mismatch
  EXPECT_TRUE(DeflateInit_.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ModuleZStream, INT32_C(-1), WasmZlibVersion,
                                   sizeof(WasmZStream) + 16},
                               RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);

  // Version Mismatch
  EXPECT_TRUE(DeflateInit_.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          ModuleZStream, INT32_C(-1), WasmZlibVersion + 2, sizeof(WasmZStream)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);

  EXPECT_TRUE(DeflateInit_.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          ModuleZStream, INT32_C(-1), WasmZlibVersion, sizeof(WasmZStream)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  WasmCompressedData = WasmHP;

  strm->AvailIn = DataSize;
  strm->NextIn = WasmData;
  strm->AvailOut = OutputBufferSize;
  strm->NextOut = WasmCompressedData;

  // deflate Test
  do {
    if (strm->AvailOut == 0) {
      WasmHP += OutputBufferSize;
      strm->AvailOut = OutputBufferSize;
      strm->NextOut = WasmHP;
    }

    EXPECT_TRUE(Deflate.run(CallFrame,
                            std::initializer_list<WasmEdge::ValVariant>{
                                ModuleZStream,
                                INT32_C(Z_FINISH),
                            },
                            RetVal));
    EXPECT_NE(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  } while (RetVal[0].get<int32_t>() != Z_STREAM_END);

  // deflateEnd Test
  EXPECT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ModuleZStream},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  WasmHP += OutputBufferSize - strm->AvailOut;
  WasmCompressedData_size = WasmHP - WasmCompressedData;
  // ----- Deflate Routine END------

  // ----- Inflate Routine START------
  fillMemContent(MemInst, ModuleZStream, sizeof(WasmZStream), 0U);

  // inflateInit_ Test
  // WASM z_stream size Mismatch
  EXPECT_TRUE(InflateInit_.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          ModuleZStream, WasmZlibVersion, sizeof(WasmZStream) + 16},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);

  // Version Mismatch
  EXPECT_TRUE(InflateInit_.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          ModuleZStream, WasmZlibVersion + 2, sizeof(WasmZStream)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);

  EXPECT_TRUE(
      InflateInit_.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           ModuleZStream, WasmZlibVersion, sizeof(WasmZStream)},
                       RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  WasmDecompressedData = WasmHP;

  strm->AvailIn = WasmCompressedData_size;
  strm->NextIn = WasmCompressedData;
  strm->AvailOut = OutputBufferSize;
  strm->NextOut = WasmDecompressedData;

  // inflate test
  do {
    if (strm->AvailOut == 0) {
      WasmHP += OutputBufferSize;
      strm->AvailOut = OutputBufferSize;
      strm->NextOut = WasmHP;
    }

    EXPECT_TRUE(Inflate.run(CallFrame,
                            std::initializer_list<WasmEdge::ValVariant>{
                                ModuleZStream,
                                INT32_C(Z_FINISH),
                            },
                            RetVal));
    EXPECT_NE(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  } while (RetVal[0].get<int32_t>() != Z_STREAM_END);

  EXPECT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ModuleZStream},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  WasmHP += OutputBufferSize - strm->AvailOut;
  WasmDecompressedDataSize = WasmHP - WasmDecompressedData;
  // ----- Inflate Routine END------

  // Test Decompressed Buffer size against source Data size.
  EXPECT_EQ(WasmDecompressedDataSize, DataSize);
  // Test Decompressed Buffer content against source Data.
  EXPECT_TRUE(std::equal(MemInst.getPointer<uint8_t *>(WasmDecompressedData),
                         MemInst.getPointer<uint8_t *>(
                             WasmDecompressedData + WasmDecompressedDataSize),
                         MemInst.getPointer<uint8_t *>(WasmData)));
}

TEST(WasmEdgeZlibTest, Module) {
  // Create the wasmedge_zlib module instance.
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  EXPECT_TRUE(ZlibMod->getEnv().ZStreamMap.empty());
  EXPECT_EQ(ZlibMod->getFuncExportNum(), 76U);

  EXPECT_NE(ZlibMod->findFuncExports("deflateInit"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflate"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflateEnd"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateInit"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflate"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateEnd"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflateInit2"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflateSetDictionary"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflateGetDictionary"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflateCopy"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflateReset"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflateParams"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflateTune"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflateBound"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflatePending"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflatePrime"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflateSetHeader"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateInit2"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateSetDictionary"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateGetDictionary"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateSync"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateCopy"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateReset"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateReset2"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflatePrime"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateMark"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateGetHeader"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateBackInit"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateBackEnd"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("zlibCompileFlags"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("compress"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("compress2"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("compressBound"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("uncompress"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("uncompress2"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzopen"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzdopen"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzbuffer"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzsetparams"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzread"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzfread"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzwrite"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzfwrite"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzputs"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzputc"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzgetc"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzungetc"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzflush"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzseek"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzrewind"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gztell"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzoffset"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzeof"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzdirect"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzclose"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzclose_r"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzclose_w"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzclearerr"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("adler32"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("adler32_z"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("adler32_combine"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("crc32"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("crc32_z"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("crc32_combine"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflateInit_"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateInit_"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflateInit2_"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateInit2_"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateBackInit_"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("gzgetc_"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateSyncPoint"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateUndermine"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateValidate"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateCodesUsed"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("inflateResetKeep"), nullptr);
  EXPECT_NE(ZlibMod->findFuncExports("deflateResetKeep"), nullptr);
}

GTEST_API_ int main(int ArgC, char **ArgV) {
  testing::InitGoogleTest(&ArgC, ArgV);
  return RUN_ALL_TESTS();
}
