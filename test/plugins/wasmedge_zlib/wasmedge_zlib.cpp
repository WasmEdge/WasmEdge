// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/defines.h"
#include "host/wasi/wasimodule.h"
#include "runtime/instance/module.h"
#include "zlibfunc.h"
#include "zlibmodule.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <sys/stat.h>
#endif

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

class WasiWiredModule : public WasmEdge::Runtime::Instance::ModuleInstance {
public:
  using ModuleInstance::ModuleInstance;
  void wireWASIModule(const ModuleInstance *M) noexcept { setWASIModule(M); }
};

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
  auto &DeflateInit_ = FuncInst->getHostFunc();

  FuncInst = ZlibMod->findFuncExports("deflate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &Deflate = FuncInst->getHostFunc();

  FuncInst = ZlibMod->findFuncExports("deflateEnd");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &DeflateEnd = FuncInst->getHostFunc();

  FuncInst = ZlibMod->findFuncExports("inflateInit_");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &InflateInit_ = FuncInst->getHostFunc();

  FuncInst = ZlibMod->findFuncExports("inflate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &Inflate = FuncInst->getHostFunc();

  FuncInst = ZlibMod->findFuncExports("inflateEnd");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &InflateEnd = FuncInst->getHostFunc();

  std::array<WasmEdge::ValVariant, 1> RetVal;

  WasmZlibVersion = WasmHP;
  std::snprintf(MemInst.getPointer<char *>(WasmHP),
                std::strlen(ZLIB_VERSION) + 1, ZLIB_VERSION);
  WasmHP += std::strlen(ZLIB_VERSION) + 1;

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
  EXPECT_TRUE(
      DeflateInit_.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           ModuleZStream, INT32_C(-1), WasmZlibVersion,
                           static_cast<uint32_t>(sizeof(WasmZStream) + 16)},
                       RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);

  // Version Mismatch
  EXPECT_TRUE(
      DeflateInit_.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           ModuleZStream, INT32_C(-1), WasmZlibVersion + 2,
                           static_cast<uint32_t>(sizeof(WasmZStream))},
                       RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);

  EXPECT_TRUE(DeflateInit_.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ModuleZStream, INT32_C(-1), WasmZlibVersion,
                                   static_cast<uint32_t>(sizeof(WasmZStream))},
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
  EXPECT_TRUE(
      InflateInit_.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           ModuleZStream, WasmZlibVersion,
                           static_cast<uint32_t>(sizeof(WasmZStream) + 16)},
                       RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);

  // Version Mismatch
  EXPECT_TRUE(InflateInit_.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ModuleZStream, WasmZlibVersion + 2,
                                   static_cast<uint32_t>(sizeof(WasmZStream))},
                               RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);

  EXPECT_TRUE(InflateInit_.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ModuleZStream, WasmZlibVersion,
                                   static_cast<uint32_t>(sizeof(WasmZStream))},
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

#define GET_ZLIB_FUNC(Var, Name)                                               \
  auto *Var##Inst = ZlibMod->findFuncExports(Name);                            \
  ASSERT_NE(Var##Inst, nullptr);                                               \
  auto &Var = Var##Inst->getHostFunc();

TEST(WasmEdgeZlibTest, HardeningBounds) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  {
    GET_ZLIB_FUNC(DeflateInit, "deflateInit")
    EXPECT_FALSE(DeflateInit.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     UINT32_C(65530), INT32_C(-1)},
                                 RetVal));
  }

  {
    GET_ZLIB_FUNC(DeflateInit, "deflateInit")
    GET_ZLIB_FUNC(Deflate, "deflate")
    const uint32_t ZS = 0;
    std::fill_n(MemInst.getPointer<uint8_t *>(ZS), sizeof(WasmZStream), 0);
    ASSERT_TRUE(DeflateInit.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-1)},
        RetVal));
    ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
    auto *Strm = MemInst.getPointer<WasmZStream *>(ZS);
    Strm->NextIn = 1000;
    Strm->AvailIn = 4;
    Strm->NextOut = 2000;
    Strm->AvailOut = UINT32_C(0xFFFFFFFF);
    EXPECT_FALSE(Deflate.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(0)},
        RetVal));
  }

  {
    GET_ZLIB_FUNC(Compress, "compress")
    EXPECT_FALSE(Compress.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(65534), UINT32_C(100), UINT32_C(10)},
        RetVal));
  }

  {
    GET_ZLIB_FUNC(Compress, "compress")
    *MemInst.getPointer<uint32_t *>(0) = 100;
    EXPECT_FALSE(Compress.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(8), UINT32_C(0), UINT32_C(4000), UINT32_C(0xFFFFFFF0)},
        RetVal));
  }

  {
    GET_ZLIB_FUNC(Uncompress, "uncompress")
    EXPECT_FALSE(Uncompress.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(65535), UINT32_C(100), UINT32_C(10)},
        RetVal));
  }

  {
    GET_ZLIB_FUNC(Adler32, "adler32")
    EXPECT_FALSE(
        Adler32.run(CallFrame,
                    std::initializer_list<WasmEdge::ValVariant>{
                        UINT32_C(1), UINT32_C(100), UINT32_C(0xFFFFFFFF)},
                    RetVal));
    GET_ZLIB_FUNC(CRC32, "crc32")
    EXPECT_FALSE(
        CRC32.run(CallFrame,
                  std::initializer_list<WasmEdge::ValVariant>{
                      UINT32_C(0), UINT32_C(100), UINT32_C(0xFFFFFFFF)},
                  RetVal));
  }

  {
    GET_ZLIB_FUNC(GZRead, "gzread")
    EXPECT_FALSE(GZRead.run(CallFrame,
                            std::initializer_list<WasmEdge::ValVariant>{
                                UINT32_C(9999), UINT32_C(0), UINT32_C(10)},
                            RetVal));
  }
}

TEST(WasmEdgeZlibTest, HardeningCompress2AnswersInvalidLevelBeforeBounds) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(Compress2, "compress2")

  const uint32_t DestLenPtr = 0;
  *MemInst.getPointer<uint32_t *>(DestLenPtr) = UINT32_C(0xFFFFFFF0);
  ASSERT_TRUE(Compress2.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(8), DestLenPtr, UINT32_C(4096), UINT32_C(64), INT32_C(10)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  EXPECT_EQ(*MemInst.getPointer<uint32_t *>(DestLenPtr), UINT32_C(0));

  *MemInst.getPointer<uint32_t *>(DestLenPtr) = UINT32_C(0xFFFFFFF0);
  EXPECT_FALSE(Compress2.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(8), DestLenPtr, UINT32_C(4096), UINT32_C(64), INT32_C(-1)},
      RetVal));
}

TEST(WasmEdgeZlibTest, HardeningUncompress2AliasedLengthMatchesNative) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(Compress, "compress")
  GET_ZLIB_FUNC(Uncompress2, "uncompress2")

  const uint32_t RawPtr = 4096;
  const uint32_t RawLen = 100;
  fillMemContent(MemInst, RawPtr, RawLen, static_cast<uint8_t>('A'));

  const uint32_t CompPtr = 256;
  const uint32_t CompCap = 512;
  const uint32_t CompLenPtr = 128;
  fillMemContent(MemInst, CompPtr, CompCap);
  *MemInst.getPointer<uint32_t *>(CompLenPtr) = CompCap;
  ASSERT_TRUE(Compress.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               CompPtr, CompLenPtr, RawPtr, RawLen},
                           RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  const uint32_t CompLen = *MemInst.getPointer<uint32_t *>(CompLenPtr);
  ASSERT_GT(CompLen, 0U);

  const uint32_t Cap = 256;
  ASSERT_GE(Cap, RawLen);
  ASSERT_GT(Cap, CompLen);
  const uint32_t OutPtr = 8192;
  const uint32_t AliasLenPtr = 64;
  fillMemContent(MemInst, OutPtr, Cap);
  *MemInst.getPointer<uint32_t *>(AliasLenPtr) = Cap;
  ASSERT_TRUE(Uncompress2.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  OutPtr, AliasLenPtr, CompPtr, AliasLenPtr},
                              RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_EQ(*MemInst.getPointer<uint32_t *>(AliasLenPtr), RawLen);
  EXPECT_EQ(0, std::memcmp(MemInst.getPointer<char *>(OutPtr),
                           MemInst.getPointer<char *>(RawPtr), RawLen));

  const uint32_t DestLenPtr = 32;
  const uint32_t SrcLenPtr = 48;
  fillMemContent(MemInst, OutPtr, Cap);
  *MemInst.getPointer<uint32_t *>(DestLenPtr) = Cap;
  *MemInst.getPointer<uint32_t *>(SrcLenPtr) = CompLen;
  ASSERT_TRUE(Uncompress2.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  OutPtr, DestLenPtr, CompPtr, SrcLenPtr},
                              RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_EQ(*MemInst.getPointer<uint32_t *>(DestLenPtr), RawLen);
  EXPECT_EQ(*MemInst.getPointer<uint32_t *>(SrcLenPtr), CompLen);
}

TEST(WasmEdgeZlibTest, HardeningZeroCapacityDestMatchesZlib) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(Compress, "compress")

  const uint32_t DestLenPtr = 0;
  const uint32_t SourcePtr = 4096;
  const char *const Payload = "zero capacity destination";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(SourcePtr), Payload);

  *MemInst.getPointer<uint32_t *>(DestLenPtr) = 0;
  ASSERT_TRUE(Compress.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               UINT32_C(0), DestLenPtr, SourcePtr, PayloadLen},
                           RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);

  *MemInst.getPointer<uint32_t *>(DestLenPtr) = 0;
  ASSERT_TRUE(
      Compress.run(CallFrame,
                   std::initializer_list<WasmEdge::ValVariant>{
                       UINT32_C(0xFFFF0000), DestLenPtr, SourcePtr, PayloadLen},
                   RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_BUF_ERROR);

  *MemInst.getPointer<uint32_t *>(DestLenPtr) = 0;
  ASSERT_TRUE(Compress.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               UINT32_C(8), DestLenPtr, SourcePtr, PayloadLen},
                           RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_BUF_ERROR);
}

TEST(WasmEdgeZlibTest, HardeningZeroLengthDictionaryMatchesZlib) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit, "deflateInit")
  GET_ZLIB_FUNC(DeflateSetDictionary, "deflateSetDictionary")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")

  const uint32_t ZS = 0;
  std::fill_n(MemInst.getPointer<uint8_t *>(ZS), sizeof(WasmZStream), 0);
  ASSERT_TRUE(DeflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-1)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  ASSERT_TRUE(
      DeflateSetDictionary.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ZS, UINT32_C(0xFFFF0000), UINT32_C(0)},
                               RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  ASSERT_TRUE(DeflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, UINT32_C(0), UINT32_C(0)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);

  EXPECT_FALSE(
      DeflateSetDictionary.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ZS, UINT32_C(0xFFFF0000), UINT32_C(64)},
                               RetVal));

  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
}

TEST(WasmEdgeZlibTest,
     HardeningInflateSetDictionaryValidatesOnlyWhenZlibReads) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit, "deflateInit")
  GET_ZLIB_FUNC(DeflateSetDictionary, "deflateSetDictionary")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")
  GET_ZLIB_FUNC(InflateInit, "inflateInit")
  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateSetDictionary, "inflateSetDictionary")
  GET_ZLIB_FUNC(Inflate, "inflate")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t OOBPtr = 0xFFFF0000;
  const uint32_t DictPtr = 0x100;
  const char *const Dict = "the quick brown fox jumps over the lazy dog";
  const uint32_t DictLen = static_cast<uint32_t>(std::strlen(Dict));
  std::strcpy(MemInst.getPointer<char *>(DictPtr), Dict);
  const uint32_t WrongDictPtr = 0x180;
  const char *const WrongDict = "an entirely different preset dictionary";
  const uint32_t WrongDictLen = static_cast<uint32_t>(std::strlen(WrongDict));
  std::strcpy(MemInst.getPointer<char *>(WrongDictPtr), WrongDict);

  const uint32_t ZS = 0x40;
  fillMemContent(MemInst, ZS, sizeof(WasmZStream));
  ASSERT_TRUE(InflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_TRUE(InflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, OOBPtr, UINT32_C(0x100)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));

  fillMemContent(MemInst, ZS, sizeof(WasmZStream));
  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-15)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_FALSE(InflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, OOBPtr, UINT32_C(0x100)},
      RetVal));
  EXPECT_TRUE(InflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, DictPtr, DictLen},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));

  const uint32_t DataPtr = 0x200;
  const char *const Payload = "the quick brown fox jumps over the lazy dog!";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);
  const uint32_t CompPtr = 0x1000;
  const uint32_t CompCap = 0x1000;
  const uint32_t DZS = 0x80;
  fillMemContent(MemInst, DZS, sizeof(WasmZStream));
  ASSERT_TRUE(DeflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, INT32_C(-1)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(DeflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, DictPtr, DictLen},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *DStrm = MemInst.getPointer<WasmZStream *>(DZS);
  DStrm->NextIn = DataPtr;
  DStrm->AvailIn = PayloadLen;
  DStrm->NextOut = CompPtr;
  DStrm->AvailOut = CompCap;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  const uint32_t CompLen = CompCap - DStrm->AvailOut;
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS}, RetVal));

  const uint32_t DecPtr = 0x2000;
  const uint32_t DecCap = 0x1000;
  fillMemContent(MemInst, ZS, sizeof(WasmZStream));
  ASSERT_TRUE(InflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *IStrm = MemInst.getPointer<WasmZStream *>(ZS);
  IStrm->NextIn = CompPtr;
  IStrm->AvailIn = CompLen;
  IStrm->NextOut = DecPtr;
  IStrm->AvailOut = DecCap;
  ASSERT_TRUE(Inflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(Z_NO_FLUSH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_NEED_DICT);

  EXPECT_FALSE(InflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, OOBPtr, UINT32_C(0x100)},
      RetVal));
  EXPECT_TRUE(
      InflateSetDictionary.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ZS, WrongDictPtr, WrongDictLen},
                               RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_DATA_ERROR);
  EXPECT_TRUE(InflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, DictPtr, DictLen},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  EXPECT_FALSE(InflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, OOBPtr, UINT32_C(0x10)},
      RetVal));

  ASSERT_TRUE(Inflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  EXPECT_EQ(
      0, std::memcmp(MemInst.getPointer<char *>(DecPtr), Payload, PayloadLen));

  EXPECT_TRUE(InflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, OOBPtr, UINT32_C(0x10)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
}

TEST(WasmEdgeZlibTest, HardeningDeflateSetDictionaryGzipStreamAnswersFirst) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetDictionary, "deflateSetDictionary")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")

  const uint32_t OOBPtr = 0xFFFF0000;
  const uint32_t ZS = 0;
  fillMemContent(MemInst, ZS, sizeof(WasmZStream));
  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           ZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_TRUE(DeflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, OOBPtr, UINT32_C(0x100)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));

  const uint32_t ZS2 = 0x40;
  fillMemContent(MemInst, ZS2, sizeof(WasmZStream));
  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           ZS2, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(15),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_FALSE(DeflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS2, OOBPtr, UINT32_C(0x100)},
      RetVal));
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS2}, RetVal));
}

TEST(WasmEdgeZlibTest, HardeningDeflateSetDictionaryStartedStreamAnswersFirst) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateSetDictionary, "deflateSetDictionary")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")

  const uint32_t OOBPtr = 0xFFFF0000;
  const uint32_t InPtr = 256;
  const uint32_t OutPtr = 1024;
  const char *const Input = "the quick brown fox jumps over the lazy dog";
  const uint32_t InLen = static_cast<uint32_t>(std::strlen(Input));
  std::strcpy(MemInst.getPointer<char *>(InPtr), Input);

  const uint32_t ZS = 0;
  fillMemContent(MemInst, ZS, sizeof(WasmZStream));
  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           ZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(15),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_FALSE(DeflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, OOBPtr, UINT32_C(0x100)},
      RetVal));

  auto *Strm = MemInst.getPointer<WasmZStream *>(ZS);
  Strm->NextIn = InPtr;
  Strm->AvailIn = InLen;
  Strm->NextOut = OutPtr;
  Strm->AvailOut = 256;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(Z_NO_FLUSH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  EXPECT_TRUE(DeflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, OOBPtr, UINT32_C(0x100)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));

  const uint32_t ZSRaw = 0x40;
  fillMemContent(MemInst, ZSRaw, sizeof(WasmZStream));
  ASSERT_TRUE(DeflateInit2.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ZSRaw, INT32_C(-1), INT32_C(Z_DEFLATED),
                                   INT32_C(-15), INT32_C(8),
                                   INT32_C(Z_DEFAULT_STRATEGY)},
                               RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *RawStrm = MemInst.getPointer<WasmZStream *>(ZSRaw);
  RawStrm->NextIn = InPtr;
  RawStrm->AvailIn = InLen;
  RawStrm->NextOut = OutPtr;
  RawStrm->AvailOut = 256;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZSRaw, INT32_C(Z_NO_FLUSH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_FALSE(
      DeflateSetDictionary.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ZSRaw, OOBPtr, UINT32_C(0x100)},
                               RetVal));
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZSRaw}, RetVal));
}

TEST(WasmEdgeZlibTest, HardeningDeflateCopyInheritsDictionaryState) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateCopy, "deflateCopy")
  GET_ZLIB_FUNC(DeflateSetDictionary, "deflateSetDictionary")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")

  const uint32_t OOBPtr = 0xFFFF0000;
  const uint32_t InPtr = 256;
  const uint32_t OutPtr = 1024;
  const uint32_t DictPtr = 2048;
  const char *const Input = "the quick brown fox jumps over the lazy dog";
  const uint32_t InLen = static_cast<uint32_t>(std::strlen(Input));
  std::strcpy(MemInst.getPointer<char *>(InPtr), Input);
  const char *const Dict = "sample preset dictionary";
  const uint32_t DictLen = static_cast<uint32_t>(std::strlen(Dict));
  std::strcpy(MemInst.getPointer<char *>(DictPtr), Dict);

  const uint32_t ZS = 0;
  const uint32_t ZC = 0x40;
  fillMemContent(MemInst, ZS, sizeof(WasmZStream));
  fillMemContent(MemInst, ZC, sizeof(WasmZStream));
  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           ZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(15),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *Strm = MemInst.getPointer<WasmZStream *>(ZS);
  Strm->NextIn = InPtr;
  Strm->AvailIn = InLen;
  Strm->NextOut = OutPtr;
  Strm->AvailOut = 512;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(Z_NO_FLUSH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(DeflateCopy.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZC, ZS}, RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_TRUE(DeflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZC, OOBPtr, UINT32_C(0x100)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZC}, RetVal));

  const uint32_t ZSRaw = 0x80;
  const uint32_t ZCRaw = 0xC0;
  fillMemContent(MemInst, ZSRaw, sizeof(WasmZStream));
  fillMemContent(MemInst, ZCRaw, sizeof(WasmZStream));
  ASSERT_TRUE(DeflateInit2.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ZSRaw, INT32_C(-1), INT32_C(Z_DEFLATED),
                                   INT32_C(-15), INT32_C(8),
                                   INT32_C(Z_DEFAULT_STRATEGY)},
                               RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(DeflateCopy.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZCRaw, ZSRaw},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *RawStrm = MemInst.getPointer<WasmZStream *>(ZCRaw);
  RawStrm->NextIn = InPtr;
  RawStrm->AvailIn = InLen;
  RawStrm->NextOut = OutPtr;
  RawStrm->AvailOut = 512;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZCRaw, INT32_C(Z_SYNC_FLUSH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_TRUE(DeflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZCRaw, DictPtr, DictLen},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZSRaw}, RetVal));
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZCRaw}, RetVal));
}

TEST(WasmEdgeZlibTest, HardeningZeroLengthChecksumMatchesZlib) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(Adler32, "adler32")
  GET_ZLIB_FUNC(CRC32, "crc32")

  const uint32_t Running = 999;
  const uint32_t WildOffset = 0xFFFF0000;

  ASSERT_TRUE(Adler32.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              Running, UINT32_C(0), UINT32_C(0)},
                          RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), INT32_C(1));

  ASSERT_TRUE(Adler32.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              Running, WildOffset, UINT32_C(0)},
                          RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(Running));

  EXPECT_FALSE(Adler32.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               Running, WildOffset, UINT32_C(1)},
                           RetVal));

  ASSERT_TRUE(CRC32.run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            Running, UINT32_C(0), UINT32_C(0)},
                        RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), INT32_C(0));

  ASSERT_TRUE(CRC32.run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            Running, WildOffset, UINT32_C(0)},
                        RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(Running));
}

TEST(WasmEdgeZlibTest, HardeningUnterminatedString) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  std::fill_n(MemInst.getPointer<uint8_t *>(0),
              static_cast<size_t>(MemInst.getSize()),
              static_cast<uint8_t>(0xFF));

  GET_ZLIB_FUNC(GZOpen, "gzopen")
  EXPECT_FALSE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1), UINT32_C(100)},
      RetVal));
}

TEST(WasmEdgeZlibTest, HardeningGZOpenRejectsOversizedPathOrMode) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(GZOpen, "gzopen")

  std::fill_n(MemInst.getPointer<uint8_t *>(0),
              static_cast<size_t>(MemInst.getSize()),
              static_cast<uint8_t>('A'));
  MemInst.getPointer<char *>(1)[5000] = '\0';
  std::strcpy(MemInst.getPointer<char *>(6000), "rb");
  EXPECT_FALSE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1), UINT32_C(6000)},
      RetVal));

  ASSERT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(6000)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);

  std::fill_n(MemInst.getPointer<uint8_t *>(0),
              static_cast<size_t>(MemInst.getSize()),
              static_cast<uint8_t>('A'));
  std::strcpy(MemInst.getPointer<char *>(1), "/tmp/x");
  MemInst.getPointer<char *>(100)[200] = '\0';
  EXPECT_FALSE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1), UINT32_C(100)},
      RetVal));
}

TEST(WasmEdgeZlibTest, GZFileRoundTrip) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  const std::string TmpPath = (std::filesystem::temp_directory_path() /
                               "wasmedge_zlib_hardening_test.gz")
                                  .string();
  ASSERT_LT(TmpPath.size() + 1, 1000U);

  const uint32_t PathPtr = 16;
  std::strcpy(MemInst.getPointer<char *>(PathPtr), TmpPath.c_str());
  const uint32_t ModeWPtr = 1024;
  std::strcpy(MemInst.getPointer<char *>(ModeWPtr), "wb");
  const uint32_t ModeRPtr = 1040;
  std::strcpy(MemInst.getPointer<char *>(ModeRPtr), "rb");
  const uint32_t DataPtr = 1100;
  const char *const Payload = "hello zlib hardening";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);

  GET_ZLIB_FUNC(GZOpen, "gzopen")
  GET_ZLIB_FUNC(GZWrite, "gzwrite")
  GET_ZLIB_FUNC(GZRead, "gzread")
  GET_ZLIB_FUNC(GZClose, "gzclose")

  if (!GZOpen.run(
          CallFrame,
          std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeWPtr},
          RetVal)) {
    GTEST_SKIP() << "cannot create temporary file: " << TmpPath;
  }
  const uint32_t WHandle = RetVal[0].get<uint32_t>();

  EXPECT_TRUE(GZWrite.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{WHandle, DataPtr, PayloadLen},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));

  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{WHandle}, RetVal));

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeRPtr},
      RetVal));
  const uint32_t RHandle = RetVal[0].get<uint32_t>();
  EXPECT_NE(RHandle, WHandle);

  EXPECT_FALSE(GZRead.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              RHandle, UINT32_C(60000), UINT32_C(0x40000000)},
                          RetVal));

  const uint32_t ReadBuf = 4096;
  EXPECT_TRUE(GZRead.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             RHandle, ReadBuf, UINT32_C(256)},
                         RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
  EXPECT_EQ(
      0, std::memcmp(MemInst.getPointer<char *>(ReadBuf), Payload, PayloadLen));

  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{RHandle}, RetVal));

  EXPECT_FALSE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{RHandle}, RetVal));

  std::remove(TmpPath.c_str());
}

TEST(WasmEdgeZlibTest, HardeningGZOversizedRequestsMatchZlib) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  const std::string TmpPath = (std::filesystem::temp_directory_path() /
                               "wasmedge_zlib_oversized_test.gz")
                                  .string();
  ASSERT_LT(TmpPath.size() + 1, 1000U);
  const uint32_t PathPtr = 16;
  std::strcpy(MemInst.getPointer<char *>(PathPtr), TmpPath.c_str());
  const uint32_t ModeWPtr = 1024;
  std::strcpy(MemInst.getPointer<char *>(ModeWPtr), "wb");
  const uint32_t ModeRPtr = 1040;
  std::strcpy(MemInst.getPointer<char *>(ModeRPtr), "rb");
  const uint32_t StrPtr = 1100;
  const char *const Payload = "gzputs through gzwrite";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(StrPtr), Payload);
  const uint32_t EmptyStrPtr = 2048;
  MemInst.getPointer<char *>(EmptyStrPtr)[0] = '\0';

  GET_ZLIB_FUNC(GZOpen, "gzopen")
  GET_ZLIB_FUNC(GZPuts, "gzputs")
  GET_ZLIB_FUNC(GZWrite, "gzwrite")
  GET_ZLIB_FUNC(GZFwrite, "gzfwrite")
  GET_ZLIB_FUNC(GZRead, "gzread")
  GET_ZLIB_FUNC(GZFread, "gzfread")
  GET_ZLIB_FUNC(GZClearerr, "gzclearerr")
  GET_ZLIB_FUNC(GZClose, "gzclose")

  const uint32_t Oversized = UINT32_C(0x80000000);

  if (!GZOpen.run(
          CallFrame,
          std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeWPtr},
          RetVal)) {
    GTEST_SKIP() << "cannot create temporary file: " << TmpPath;
  }
  const uint32_t WHandle = RetVal[0].get<uint32_t>();

  ASSERT_TRUE(GZPuts.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{WHandle, StrPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));

  ASSERT_TRUE(GZPuts.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{WHandle, EmptyStrPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), INT32_C(0));

  ASSERT_TRUE(GZFwrite.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               StrPtr, Oversized, UINT32_C(2), WHandle},
                           RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), INT32_C(0));
  ASSERT_TRUE(GZPuts.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{WHandle, EmptyStrPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), INT32_C(-1));
  ASSERT_TRUE(GZClearerr.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{WHandle}, {}));
  ASSERT_TRUE(GZPuts.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{WHandle, EmptyStrPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), INT32_C(0));

  ASSERT_TRUE(GZWrite.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{WHandle, StrPtr, Oversized},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), INT32_C(0));

  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{WHandle}, RetVal));

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeRPtr},
      RetVal));
  const uint32_t RHandle = RetVal[0].get<uint32_t>();

  const uint32_t ReadBuf = 4096;
  ASSERT_TRUE(GZRead.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             RHandle, ReadBuf, UINT32_C(256)},
                         RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
  EXPECT_EQ(
      0, std::memcmp(MemInst.getPointer<char *>(ReadBuf), Payload, PayloadLen));

  ASSERT_TRUE(GZFread.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              ReadBuf, Oversized, UINT32_C(2), RHandle},
                          RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), INT32_C(0));
  ASSERT_TRUE(GZRead.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             RHandle, ReadBuf, UINT32_C(8)},
                         RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), INT32_C(-1));
  ASSERT_TRUE(GZClearerr.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{RHandle}, {}));
  ASSERT_TRUE(GZRead.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             RHandle, ReadBuf, UINT32_C(8)},
                         RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), INT32_C(0));

  ASSERT_TRUE(GZRead.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{RHandle, ReadBuf, Oversized},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), INT32_C(-1));

  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{RHandle}, RetVal));
  std::remove(TmpPath.c_str());
}

TEST(WasmEdgeZlibTest, HardeningGZPutsEmptyStringHonorsMode) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  const std::string TmpPath =
      (std::filesystem::temp_directory_path() / "wasmedge_zlib_gzputs_mode.gz")
          .string();
  ASSERT_LT(TmpPath.size() + 1, 1000U);

  const uint32_t PathPtr = 16;
  std::strcpy(MemInst.getPointer<char *>(PathPtr), TmpPath.c_str());
  const uint32_t ModeWPtr = 1024;
  std::strcpy(MemInst.getPointer<char *>(ModeWPtr), "wb");
  const uint32_t ModeRPtr = 1040;
  std::strcpy(MemInst.getPointer<char *>(ModeRPtr), "rb");
  const uint32_t EmptyPtr = 1060;
  MemInst.getPointer<char *>(EmptyPtr)[0] = '\0';
  const uint32_t DataPtr = 1100;
  const char *const Payload = "gzputs mode probe";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);

  GET_ZLIB_FUNC(GZOpen, "gzopen")
  GET_ZLIB_FUNC(GZPuts, "gzputs")
  GET_ZLIB_FUNC(GZRead, "gzread")
  GET_ZLIB_FUNC(GZClose, "gzclose")

  if (!GZOpen.run(
          CallFrame,
          std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeWPtr},
          RetVal)) {
    GTEST_SKIP() << "cannot create temporary file: " << TmpPath;
  }
  const uint32_t WHandle = RetVal[0].get<uint32_t>();
  ASSERT_NE(WHandle, 0U);

  EXPECT_TRUE(GZPuts.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{WHandle, EmptyPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 0);
  EXPECT_TRUE(GZPuts.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{WHandle, DataPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{WHandle}, RetVal));

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeRPtr},
      RetVal));
  const uint32_t RHandle = RetVal[0].get<uint32_t>();
  ASSERT_NE(RHandle, 0U);

  EXPECT_TRUE(GZPuts.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{RHandle, EmptyPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), -1);
  EXPECT_TRUE(GZPuts.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{RHandle, DataPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), -1);

  const uint32_t ReadBuf = 4096;
  EXPECT_TRUE(GZRead.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             RHandle, ReadBuf, UINT32_C(256)},
                         RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
  EXPECT_EQ(
      0, std::memcmp(MemInst.getPointer<char *>(ReadBuf), Payload, PayloadLen));

  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{RHandle}, RetVal));
  std::remove(TmpPath.c_str());
}

TEST(WasmEdgeZlibTest, GZHeaderSnapshot) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetHeader, "deflateSetHeader")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")
  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetHeader, "inflateGetHeader")
  GET_ZLIB_FUNC(Inflate, "inflate")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t DZS = 0x100;
  const uint32_t DHdr = 0x200;
  const uint32_t NamePtr = 0x300;
  const uint32_t DataPtr = 0x500;
  const uint32_t CompPtr = 0x1000;
  const uint32_t CompCap = 0x2000;
  const char *const OrigName = "original-header-name";
  const char *const Payload = "payload-to-compress-through-gzip";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(NamePtr), OrigName);
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);
  std::fill_n(MemInst.getPointer<uint8_t *>(DZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(DHdr), sizeof(WasmGZHeader), 0);

  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *DHeader = MemInst.getPointer<WasmGZHeader *>(DHdr);
  DHeader->Name = NamePtr;
  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, DHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  std::strcpy(MemInst.getPointer<char *>(NamePtr), "CORRUPTED-VALUE");
  DHeader->Name = static_cast<uint32_t>(MemInst.getSize()) - 3;
  std::fill_n(MemInst.getPointer<uint8_t *>(
                  static_cast<uint32_t>(MemInst.getSize()) - 3),
              3, static_cast<uint8_t>(0xFF));

  auto *DStrm = MemInst.getPointer<WasmZStream *>(DZS);
  DStrm->NextIn = DataPtr;
  DStrm->AvailIn = PayloadLen;
  DStrm->NextOut = CompPtr;
  DStrm->AvailOut = CompCap;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  const uint32_t CompLen = CompCap - DStrm->AvailOut;
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS}, RetVal));

  const uint32_t IZS = 0x4000;
  const uint32_t IHdr = 0x4100;
  const uint32_t INameBuf = 0x4200;
  const uint32_t INameMax = 128;
  const uint32_t DecPtr = 0x5000;
  const uint32_t DecCap = 0x1000;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(IHdr), sizeof(WasmGZHeader), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(INameBuf), INameMax, 0);

  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IHeader = MemInst.getPointer<WasmGZHeader *>(IHdr);
  IHeader->Name = INameBuf;
  IHeader->NameMax = INameMax;
  ASSERT_TRUE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, IHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IStrm = MemInst.getPointer<WasmZStream *>(IZS);
  IStrm->NextIn = CompPtr;
  IStrm->AvailIn = CompLen;
  IStrm->NextOut = DecPtr;
  IStrm->AvailOut = DecCap;
  ASSERT_TRUE(Inflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));

  EXPECT_STREQ(MemInst.getPointer<char *>(INameBuf), OrigName);
}

TEST(WasmEdgeZlibTest, GZOpenFailureReturnsNullHandle) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  const std::string MissingPath =
      (std::filesystem::temp_directory_path() / "wasmedge_zlib_absent_dir_zzz" /
       "definitely_absent.gz")
          .string();
  ASSERT_LT(MissingPath.size() + 1, 900U);
  const uint32_t PathPtr = 16;
  std::strcpy(MemInst.getPointer<char *>(PathPtr), MissingPath.c_str());
  const uint32_t ModePtr = 1000;
  std::strcpy(MemInst.getPointer<char *>(ModePtr), "rb");

  GET_ZLIB_FUNC(GZOpen, "gzopen")
  EXPECT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModePtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);

  const uint32_t ModeDPtr = 1010;
  std::strcpy(MemInst.getPointer<char *>(ModeDPtr), "rb");
  GET_ZLIB_FUNC(GZDOpen, "gzdopen")
  EXPECT_TRUE(GZDOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{INT32_C(-1), ModeDPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);
}

TEST(WasmEdgeZlibTest, DeflateSetHeaderKeepsHeaderOnFailedReplace) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetHeader, "deflateSetHeader")

  const uint32_t DZS = 0x100;
  const uint32_t DHdr = 0x200;
  const uint32_t Name1Ptr = 0x300;
  const char *const Name1 = "original-header-name";
  std::strcpy(MemInst.getPointer<char *>(Name1Ptr), Name1);
  std::fill_n(MemInst.getPointer<uint8_t *>(DZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(DHdr), sizeof(WasmGZHeader), 0);

  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *DHeader = MemInst.getPointer<WasmGZHeader *>(DHdr);
  DHeader->Name = Name1Ptr;
  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, DHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  DHeader->Name = static_cast<uint32_t>(MemInst.getSize()) - 3;
  std::fill_n(MemInst.getPointer<uint8_t *>(
                  static_cast<uint32_t>(MemInst.getSize()) - 3),
              3, static_cast<uint8_t>(0xFF));
  EXPECT_FALSE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, DHdr},
      RetVal));

  const auto &HeaderMap = ZlibMod->getEnv().GZHeaderMap;
  const auto StoreIt = HeaderMap.find(DZS);
  ASSERT_NE(StoreIt, HeaderMap.end());
  EXPECT_EQ(StoreIt->second->Name, Name1);
}

TEST(WasmEdgeZlibTest, DeflateSetHeaderRejectsOversizedNameAndComment) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(18, 18)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetHeader, "deflateSetHeader")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")

  constexpr uint32_t TooLongHeaderStringLen = (UINT32_C(1) << 20) + 1;
  const uint32_t LongStringPtr = 0x1000;
  std::fill_n(MemInst.getPointer<char *>(LongStringPtr), TooLongHeaderStringLen,
              'h');
  *MemInst.getPointer<char *>(LongStringPtr + TooLongHeaderStringLen) = '\0';

  const auto InitGzipStream = [&](uint32_t ZS) {
    std::fill_n(MemInst.getPointer<uint8_t *>(ZS), sizeof(WasmZStream), 0);
    ASSERT_TRUE(
        DeflateInit2.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             ZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                             INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                         RetVal));
    ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  };

  const uint32_t NameZS = 0x100;
  const uint32_t NameHdr = 0x200;
  InitGzipStream(NameZS);
  std::fill_n(MemInst.getPointer<uint8_t *>(NameHdr), sizeof(WasmGZHeader), 0);
  auto *NameHeader = MemInst.getPointer<WasmGZHeader *>(NameHdr);
  NameHeader->Name = LongStringPtr;
  EXPECT_FALSE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{NameZS, NameHdr},
      RetVal));
  EXPECT_EQ(ZlibMod->getEnv().GZHeaderMap.count(NameZS), 0U);
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{NameZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  const uint32_t CommentZS = 0x300;
  const uint32_t CommentHdr = 0x400;
  InitGzipStream(CommentZS);
  std::fill_n(MemInst.getPointer<uint8_t *>(CommentHdr), sizeof(WasmGZHeader),
              0);
  auto *CommentHeader = MemInst.getPointer<WasmGZHeader *>(CommentHdr);
  CommentHeader->Comment = LongStringPtr;
  EXPECT_FALSE(DeflateSetHeader.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{CommentZS, CommentHdr},
      RetVal));
  EXPECT_EQ(ZlibMod->getEnv().GZHeaderMap.count(CommentZS), 0U);
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{CommentZS},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
}

TEST(WasmEdgeZlibTest, HardeningDeflateSetHeaderNonGzipSkipsSnapshot) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetHeader, "deflateSetHeader")

  const uint32_t DZS = 0x100;
  const uint32_t DHdr = 0x200;
  std::fill_n(MemInst.getPointer<uint8_t *>(DZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(DHdr), sizeof(WasmGZHeader), 0);

  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(15),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *DHeader = MemInst.getPointer<WasmGZHeader *>(DHdr);
  DHeader->Name = static_cast<uint32_t>(MemInst.getSize()) - 3;
  std::fill_n(MemInst.getPointer<uint8_t *>(
                  static_cast<uint32_t>(MemInst.getSize()) - 3),
              3, static_cast<uint8_t>(0xFF));
  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, DHdr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  EXPECT_EQ(ZlibMod->getEnv().GZHeaderMap.count(DZS), 0U);
}

TEST(WasmEdgeZlibTest, EndErasesGzipHeaderSnapshot) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetHeader, "deflateSetHeader")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")
  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetHeader, "inflateGetHeader")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const auto &HeaderMap = ZlibMod->getEnv().GZHeaderMap;

  const uint32_t DZS = 0x100;
  const uint32_t DHdr = 0x200;
  const uint32_t NamePtr = 0x300;
  std::strcpy(MemInst.getPointer<char *>(NamePtr), "deflate-header-name");
  std::fill_n(MemInst.getPointer<uint8_t *>(DZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(DHdr), sizeof(WasmGZHeader), 0);

  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  MemInst.getPointer<WasmGZHeader *>(DHdr)->Name = NamePtr;
  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, DHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_NE(HeaderMap.find(DZS), HeaderMap.end());

  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS}, RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_EQ(HeaderMap.find(DZS), HeaderMap.end());

  const uint32_t IZS = 0x2000;
  const uint32_t IHdr = 0x2100;
  const uint32_t INameBuf = 0x2200;
  const uint32_t INameMax = 128;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(IHdr), sizeof(WasmGZHeader), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(INameBuf), INameMax, 0);

  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IHeader = MemInst.getPointer<WasmGZHeader *>(IHdr);
  IHeader->Name = INameBuf;
  IHeader->NameMax = INameMax;
  ASSERT_TRUE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, IHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_NE(HeaderMap.find(IZS), HeaderMap.end());

  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_EQ(HeaderMap.find(IZS), HeaderMap.end());
}

TEST(WasmEdgeZlibTest, InflateResetDropsGzipHeaderSnapshot) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")
  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetHeader, "inflateGetHeader")
  GET_ZLIB_FUNC(InflateReset, "inflateReset")
  GET_ZLIB_FUNC(InflateReset2, "inflateReset2")
  GET_ZLIB_FUNC(InflateResetKeep, "inflateResetKeep")
  GET_ZLIB_FUNC(Inflate, "inflate")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t DZS = 0x100;
  const uint32_t DataPtr = 0x300;
  const uint32_t CompPtr = 0x1000;
  const uint32_t CompCap = 0x2000;
  const char *const Payload = "payload-for-inflate-reset";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::fill_n(MemInst.getPointer<uint8_t *>(DZS), sizeof(WasmZStream), 0);
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);

  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *DStrm = MemInst.getPointer<WasmZStream *>(DZS);
  DStrm->NextIn = DataPtr;
  DStrm->AvailIn = PayloadLen;
  DStrm->NextOut = CompPtr;
  DStrm->AvailOut = CompCap;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  const uint32_t CompLen = CompCap - DStrm->AvailOut;
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS}, RetVal));

  const auto &HeaderMap = ZlibMod->getEnv().GZHeaderMap;

  const uint32_t IZS = 0x4000;
  const uint32_t IHdr = 0x4100;
  const uint32_t NameBuf = 0x4200;
  const uint32_t NameMax = 64;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(IHdr), sizeof(WasmGZHeader), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(NameBuf), NameMax, 0);

  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IHeader = MemInst.getPointer<WasmGZHeader *>(IHdr);
  const auto RegisterHeader = [&]() {
    IHeader->Name = NameBuf;
    IHeader->NameMax = NameMax;
    ASSERT_TRUE(InflateGetHeader.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, IHdr},
        RetVal));
    ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
    ASSERT_NE(HeaderMap.find(IZS), HeaderMap.end());
  };

  RegisterHeader();
  ASSERT_TRUE(InflateReset.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_EQ(HeaderMap.find(IZS), HeaderMap.end());

  RegisterHeader();
  ASSERT_TRUE(InflateReset2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_EQ(HeaderMap.find(IZS), HeaderMap.end());

  RegisterHeader();
  ASSERT_TRUE(InflateResetKeep.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_EQ(HeaderMap.find(IZS), HeaderMap.end());

  RegisterHeader();
  ASSERT_TRUE(InflateReset.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  IHeader->Name = 0xFFFFFF00;
  IHeader->NameMax = 0xFFFFFFFF;

  const uint32_t DecPtr = 0x5000;
  const uint32_t DecCap = 0x1000;
  auto *IStrm = MemInst.getPointer<WasmZStream *>(IZS);
  IStrm->NextIn = CompPtr;
  IStrm->AvailIn = CompLen;
  IStrm->NextOut = DecPtr;
  IStrm->AvailOut = DecCap;
  ASSERT_TRUE(Inflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(Z_FINISH)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  EXPECT_EQ(
      std::memcmp(MemInst.getPointer<char *>(DecPtr), Payload, PayloadLen), 0);
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
}

TEST(WasmEdgeZlibTest, DeflateCopyKeepsCopiedHeaderAlive) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetHeader, "deflateSetHeader")
  GET_ZLIB_FUNC(DeflateCopy, "deflateCopy")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")
  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetHeader, "inflateGetHeader")
  GET_ZLIB_FUNC(Inflate, "inflate")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t SrcZS = 0x100;
  const uint32_t DstZS = 0x180;
  const uint32_t Hdr = 0x200;
  const uint32_t NamePtr = 0x300;
  const uint32_t Name2Ptr = 0x340;
  const uint32_t DataPtr = 0x400;
  const uint32_t CompPtr = 0x1000;
  const uint32_t CompCap = 0x3000;
  const char *const OrigName = "original-header-name";
  const char *const NewName = "replacement-header-name";
  const char *const Payload = "payload-through-copied-gzip-stream";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(NamePtr), OrigName);
  std::strcpy(MemInst.getPointer<char *>(Name2Ptr), NewName);
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);
  std::fill_n(MemInst.getPointer<uint8_t *>(SrcZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(DstZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(Hdr), sizeof(WasmGZHeader), 0);

  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           SrcZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *DHeader = MemInst.getPointer<WasmGZHeader *>(Hdr);
  DHeader->Name = NamePtr;
  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{SrcZS, Hdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  ASSERT_TRUE(DeflateCopy.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DstZS, SrcZS},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  const auto &HeaderMap = ZlibMod->getEnv().GZHeaderMap;
  {
    const auto DstIt = HeaderMap.find(DstZS);
    ASSERT_NE(DstIt, HeaderMap.end());
    EXPECT_EQ(DstIt->second->Name, OrigName);
  }

  DHeader->Name = Name2Ptr;
  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{SrcZS, Hdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  {
    const auto DstIt = HeaderMap.find(DstZS);
    ASSERT_NE(DstIt, HeaderMap.end());
    EXPECT_EQ(DstIt->second->Name, OrigName);
  }

  auto *DStrm = MemInst.getPointer<WasmZStream *>(DstZS);
  DStrm->NextIn = DataPtr;
  DStrm->AvailIn = PayloadLen;
  DStrm->NextOut = CompPtr;
  DStrm->AvailOut = CompCap;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DstZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  const uint32_t CompLen = CompCap - DStrm->AvailOut;
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DstZS}, RetVal));
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{SrcZS}, RetVal));

  const uint32_t IZS = 0x5000;
  const uint32_t IHdr = 0x5100;
  const uint32_t INameBuf = 0x5200;
  const uint32_t INameMax = 128;
  const uint32_t DecPtr = 0x6000;
  const uint32_t DecCap = 0x2000;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(IHdr), sizeof(WasmGZHeader), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(INameBuf), INameMax, 0);

  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IHeader = MemInst.getPointer<WasmGZHeader *>(IHdr);
  IHeader->Name = INameBuf;
  IHeader->NameMax = INameMax;
  ASSERT_TRUE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, IHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IStrm = MemInst.getPointer<WasmZStream *>(IZS);
  IStrm->NextIn = CompPtr;
  IStrm->AvailIn = CompLen;
  IStrm->NextOut = DecPtr;
  IStrm->AvailOut = DecCap;
  ASSERT_TRUE(Inflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));

  EXPECT_STREQ(MemInst.getPointer<char *>(INameBuf), OrigName);
}

TEST(WasmEdgeZlibTest, InflateCopyKeepsCopiedHeaderAlive) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetHeader, "inflateGetHeader")
  GET_ZLIB_FUNC(InflateCopy, "inflateCopy")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t SrcZS = 0x100;
  const uint32_t DstZS = 0x180;
  const uint32_t Hdr = 0x200;
  const uint32_t NameBuf = 0x300;
  const uint32_t NameMax = 128;
  std::fill_n(MemInst.getPointer<uint8_t *>(SrcZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(DstZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(Hdr), sizeof(WasmGZHeader), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(NameBuf), NameMax, 0);

  ASSERT_TRUE(InflateInit2.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{SrcZS, INT32_C(31)}, RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IHeader = MemInst.getPointer<WasmGZHeader *>(Hdr);
  IHeader->Name = NameBuf;
  IHeader->NameMax = NameMax;
  ASSERT_TRUE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{SrcZS, Hdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  ASSERT_TRUE(InflateCopy.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DstZS, SrcZS},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  const auto &HeaderMap = ZlibMod->getEnv().GZHeaderMap;
  const auto SrcIt = HeaderMap.find(SrcZS);
  const auto DstIt = HeaderMap.find(DstZS);
  ASSERT_NE(SrcIt, HeaderMap.end());
  ASSERT_NE(DstIt, HeaderMap.end());
  EXPECT_EQ(DstIt->second, SrcIt->second);

  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{SrcZS}, RetVal));
  EXPECT_EQ(HeaderMap.find(SrcZS), HeaderMap.end());
  EXPECT_NE(HeaderMap.find(DstZS), HeaderMap.end());

  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DstZS}, RetVal));
}

TEST(WasmEdgeZlibTest, CopyInitializesDirtyDestinationStream) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateCopy, "deflateCopy")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")
  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(Inflate, "inflate")
  GET_ZLIB_FUNC(InflateCopy, "inflateCopy")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t DataPtr = 0x300;
  const char *const Payload = "payload-for-copy-write-back";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);

  const uint32_t AZS = 0x200;
  const uint32_t BlobPtr = 0x3000;
  const uint32_t BlobCap = 0x1000;
  std::fill_n(MemInst.getPointer<uint8_t *>(AZS), sizeof(WasmZStream), 0);
  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           AZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *AStrm = MemInst.getPointer<WasmZStream *>(AZS);
  AStrm->NextIn = DataPtr;
  AStrm->AvailIn = PayloadLen;
  AStrm->NextOut = BlobPtr;
  AStrm->AvailOut = BlobCap;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{AZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  const uint32_t BlobLen = BlobCap - AStrm->AvailOut;
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{AZS}, RetVal));

  const uint32_t SZS = 0x100;
  const uint32_t DCopyZS = 0x180;
  const uint32_t CompPtr = 0x1000;
  const uint32_t CompCap = 0x2000;
  std::fill_n(MemInst.getPointer<uint8_t *>(SZS), sizeof(WasmZStream), 0);
  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           SZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(15),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *SStrm = MemInst.getPointer<WasmZStream *>(SZS);
  SStrm->NextIn = DataPtr;
  SStrm->AvailIn = PayloadLen;
  SStrm->NextOut = CompPtr;
  SStrm->AvailOut = CompCap;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{SZS, INT32_C(Z_NO_FLUSH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  SStrm->NextIn = DataPtr;
  SStrm->AvailIn = PayloadLen;

  std::fill_n(MemInst.getPointer<uint8_t *>(DCopyZS), sizeof(WasmZStream),
              0xFF);
  ASSERT_TRUE(DeflateCopy.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DCopyZS, SZS},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *DCopyStrm = MemInst.getPointer<WasmZStream *>(DCopyZS);
  EXPECT_EQ(DCopyStrm->NextIn, SStrm->NextIn);
  EXPECT_EQ(DCopyStrm->AvailIn, SStrm->AvailIn);
  EXPECT_EQ(DCopyStrm->TotalIn, SStrm->TotalIn);
  EXPECT_EQ(DCopyStrm->NextOut, SStrm->NextOut);
  EXPECT_EQ(DCopyStrm->AvailOut, SStrm->AvailOut);
  EXPECT_EQ(DCopyStrm->TotalOut, SStrm->TotalOut);

  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DCopyZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DCopyZS}, RetVal));
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{SZS}, RetVal));

  const uint32_t IZS = 0x4000;
  const uint32_t ICopyZS = 0x4080;
  const uint32_t DecPtr = 0x5000;
  const uint32_t DecCap = 0x1000;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *IStrm = MemInst.getPointer<WasmZStream *>(IZS);
  IStrm->NextIn = BlobPtr;
  IStrm->AvailIn = 12;
  IStrm->NextOut = DecPtr;
  IStrm->AvailOut = DecCap;
  ASSERT_TRUE(Inflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(Z_NO_FLUSH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  IStrm->AvailIn = BlobLen - 12;

  std::fill_n(MemInst.getPointer<uint8_t *>(ICopyZS), sizeof(WasmZStream),
              0xFF);
  ASSERT_TRUE(InflateCopy.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ICopyZS, IZS},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *ICopyStrm = MemInst.getPointer<WasmZStream *>(ICopyZS);
  EXPECT_EQ(ICopyStrm->NextIn, IStrm->NextIn);
  EXPECT_EQ(ICopyStrm->AvailIn, IStrm->AvailIn);
  EXPECT_EQ(ICopyStrm->TotalIn, IStrm->TotalIn);
  EXPECT_EQ(ICopyStrm->NextOut, IStrm->NextOut);
  EXPECT_EQ(ICopyStrm->AvailOut, IStrm->AvailOut);
  EXPECT_EQ(ICopyStrm->TotalOut, IStrm->TotalOut);

  ASSERT_TRUE(Inflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ICopyZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  EXPECT_EQ(
      std::memcmp(MemInst.getPointer<char *>(DecPtr), Payload, PayloadLen), 0);
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ICopyZS}, RetVal));
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
}

TEST(WasmEdgeZlibTest, InflateGetHeaderReplacesStoredHeaderOnSuccess) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetHeader, "inflateGetHeader")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t IZS = 0x100;
  const uint32_t Hdr1 = 0x200;
  const uint32_t Hdr2 = 0x280;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(Hdr1), sizeof(WasmGZHeader), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(Hdr2), sizeof(WasmGZHeader), 0);

  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  ASSERT_TRUE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, Hdr1},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  const auto &HeaderMap = ZlibMod->getEnv().GZHeaderMap;
  {
    const auto It = HeaderMap.find(IZS);
    ASSERT_NE(It, HeaderMap.end());
    EXPECT_EQ(It->second->WasmGZHeaderOffset, Hdr1);
  }

  ASSERT_TRUE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, Hdr2},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  {
    const auto It = HeaderMap.find(IZS);
    ASSERT_NE(It, HeaderMap.end());
    EXPECT_EQ(It->second->WasmGZHeaderOffset, Hdr2);
  }

  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
}

TEST(WasmEdgeZlibTest, DeflateEndEarlyAbortErasesState) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetHeader, "deflateSetHeader")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")

  const uint32_t DZS = 0x100;
  const uint32_t Hdr = 0x200;
  const uint32_t NamePtr = 0x300;
  const uint32_t DataPtr = 0x400;
  const uint32_t CompPtr = 0x1000;
  const uint32_t CompCap = 0x1000;
  const char *const Payload = "partial-payload";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(NamePtr), "aborted-stream-name");
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);
  std::fill_n(MemInst.getPointer<uint8_t *>(DZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(Hdr), sizeof(WasmGZHeader), 0);

  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  MemInst.getPointer<WasmGZHeader *>(Hdr)->Name = NamePtr;
  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, Hdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  const auto &Env = ZlibMod->getEnv();
  ASSERT_NE(Env.ZStreamMap.find(DZS), Env.ZStreamMap.end());
  ASSERT_NE(Env.GZHeaderMap.find(DZS), Env.GZHeaderMap.end());

  auto *DStrm = MemInst.getPointer<WasmZStream *>(DZS);
  DStrm->NextIn = DataPtr;
  DStrm->AvailIn = PayloadLen;
  DStrm->NextOut = CompPtr;
  DStrm->AvailOut = CompCap;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, INT32_C(Z_NO_FLUSH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS}, RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_DATA_ERROR);
  EXPECT_EQ(Env.ZStreamMap.find(DZS), Env.ZStreamMap.end());
  EXPECT_EQ(Env.GZHeaderMap.find(DZS), Env.GZHeaderMap.end());
}

TEST(WasmEdgeZlibTest, InflateSyncAllowsDirtyOutputBuffer) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateSync, "inflateSync")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t IZS = 0x100;
  const uint32_t InPtr = 0x200;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(InPtr), 8,
              static_cast<uint8_t>(0xFF));

  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IStrm = MemInst.getPointer<WasmZStream *>(IZS);
  IStrm->NextIn = InPtr;
  IStrm->AvailIn = 8;
  IStrm->NextOut = static_cast<uint32_t>(MemInst.getSize());
  IStrm->AvailOut = 16;
  EXPECT_TRUE(InflateSync.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_DATA_ERROR);

  IStrm->NextIn = static_cast<uint32_t>(MemInst.getSize());
  IStrm->AvailIn = 8;
  IStrm->NextOut = 0;
  IStrm->AvailOut = 0;
  EXPECT_FALSE(InflateSync.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));

  IStrm->NextIn = InPtr;
  IStrm->AvailIn = 0;
  IStrm->NextOut = 0;
  IStrm->AvailOut = 0;
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
}

TEST(WasmEdgeZlibTest, ControlCallsIgnoreDirtyDataBuffers) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit, "deflateInit")
  GET_ZLIB_FUNC(DeflateReset, "deflateReset")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")

  const uint32_t MemBytes = static_cast<uint32_t>(MemInst.getSize());
  const uint32_t ZS = 0;
  std::fill_n(MemInst.getPointer<uint8_t *>(ZS), sizeof(WasmZStream), 0);

  auto SetDirtyBuffers = [&]() {
    auto *Strm = MemInst.getPointer<WasmZStream *>(ZS);
    Strm->NextIn = MemBytes - 4;
    Strm->AvailIn = 0x10000;
    Strm->NextOut = MemBytes - 4;
    Strm->AvailOut = 0x10000;
  };

  SetDirtyBuffers();
  ASSERT_TRUE(DeflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-1)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  SetDirtyBuffers();
  ASSERT_TRUE(DeflateReset.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  {
    auto *Strm = MemInst.getPointer<WasmZStream *>(ZS);
    Strm->NextIn = 0;
    Strm->AvailIn = 0;
    Strm->NextOut = MemBytes - 4;
    Strm->AvailOut = 0x10000;
  }
  EXPECT_FALSE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(Z_NO_FLUSH)},
      RetVal));

  SetDirtyBuffers();
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
}

TEST(WasmEdgeZlibTest, HardeningRejectedParamsIgnoreDirtyDataBuffers) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit, "deflateInit")
  GET_ZLIB_FUNC(DeflateParams, "deflateParams")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")

  const uint32_t MemBytes = static_cast<uint32_t>(MemInst.getSize());
  const uint32_t ZS = 0;
  std::fill_n(MemInst.getPointer<uint8_t *>(ZS), sizeof(WasmZStream), 0);

  ASSERT_TRUE(DeflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-1)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto SetDirtyBuffers = [&]() {
    auto *Strm = MemInst.getPointer<WasmZStream *>(ZS);
    Strm->NextIn = MemBytes - 4;
    Strm->AvailIn = 0x10000;
    Strm->NextOut = MemBytes - 4;
    Strm->AvailOut = 0x10000;
  };

  SetDirtyBuffers();
  ASSERT_TRUE(DeflateParams.run(CallFrame,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    ZS, INT32_C(10), INT32_C(Z_FILTERED)},
                                RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);

  SetDirtyBuffers();
  ASSERT_TRUE(DeflateParams.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-1), INT32_C(99)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);

  SetDirtyBuffers();
  ASSERT_TRUE(Deflate.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(99)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);

  SetDirtyBuffers();
  EXPECT_FALSE(DeflateParams.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     ZS, INT32_C(1), INT32_C(Z_FILTERED)},
                                 RetVal));

  SetDirtyBuffers();
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
}

TEST(WasmEdgeZlibTest, Module) {
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

TEST(WasmEdgeZlibTest, HardeningInflateGetHeaderValidatesHeadPtr) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetHeader, "inflateGetHeader")

  const uint32_t IZS = 0;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  EXPECT_FALSE(InflateGetHeader.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{IZS, UINT32_C(65530)},
      RetVal));
}

TEST(WasmEdgeZlibTest, HardeningInflateGetHeaderAnswersNonGzipStreams) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(InflateInit, "inflateInit")
  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetHeader, "inflateGetHeader")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t OOBHeadPtr = 65530;

  const uint32_t ZS = 0;
  fillMemContent(MemInst, ZS, sizeof(WasmZStream));
  ASSERT_TRUE(InflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_TRUE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, OOBHeadPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));

  fillMemContent(MemInst, ZS, sizeof(WasmZStream));
  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-15)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_TRUE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, OOBHeadPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));

  fillMemContent(MemInst, ZS, sizeof(WasmZStream));
  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(47)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_FALSE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, OOBHeadPtr},
      RetVal));
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
}

TEST(WasmEdgeZlibTest, HardeningInflateEndToleratesCorruptHeaderMax) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetHeader, "inflateGetHeader")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t IZS = 0;
  const uint32_t IHdr = 0x100;
  const uint32_t NameBuf = 0x200;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(IHdr), sizeof(WasmGZHeader), 0);

  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IHeader = MemInst.getPointer<WasmGZHeader *>(IHdr);
  IHeader->Name = NameBuf;
  IHeader->NameMax = 64;
  ASSERT_TRUE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, IHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  IHeader->NameMax = UINT32_C(0xFFFFFFFF);
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
}

TEST(WasmEdgeZlibTest, GZHeaderAbsentOptionalFieldsSurfaceAsNull) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")
  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetHeader, "inflateGetHeader")
  GET_ZLIB_FUNC(Inflate, "inflate")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t DZS = 0x100;
  const uint32_t DataPtr = 0x500;
  const uint32_t CompPtr = 0x1000;
  const uint32_t CompCap = 0x2000;
  const char *const Payload = "absent-header-fields-payload";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);
  std::fill_n(MemInst.getPointer<uint8_t *>(DZS), sizeof(WasmZStream), 0);

  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *DStrm = MemInst.getPointer<WasmZStream *>(DZS);
  DStrm->NextIn = DataPtr;
  DStrm->AvailIn = PayloadLen;
  DStrm->NextOut = CompPtr;
  DStrm->AvailOut = CompCap;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  const uint32_t CompLen = CompCap - DStrm->AvailOut;
  ASSERT_GT(CompLen, 12U);
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS}, RetVal));

  const uint32_t IZS = 0x4000;
  const uint32_t IHdr = 0x4100;
  const uint32_t ExtraBuf = 0x4200;
  const uint32_t NameBuf = 0x4300;
  const uint32_t CommBuf = 0x4400;
  const uint32_t DecPtr = 0x5000;
  const uint32_t DecCap = 0x1000;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(IHdr), sizeof(WasmGZHeader), 0);

  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IHeader = MemInst.getPointer<WasmGZHeader *>(IHdr);
  IHeader->Extra = ExtraBuf;
  IHeader->ExtraMax = 64;
  IHeader->Name = NameBuf;
  IHeader->NameMax = 64;
  IHeader->Comment = CommBuf;
  IHeader->CommMax = 64;
  ASSERT_TRUE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, IHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IStrm = MemInst.getPointer<WasmZStream *>(IZS);
  IStrm->NextIn = CompPtr;
  IStrm->AvailIn = 12;
  IStrm->NextOut = DecPtr;
  IStrm->AvailOut = DecCap;
  ASSERT_TRUE(Inflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(Z_NO_FLUSH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_EQ(IHeader->Extra, 0U);
  EXPECT_EQ(IHeader->Name, 0U);
  EXPECT_EQ(IHeader->Comment, 0U);
  EXPECT_EQ(IHeader->Done, 1);

  IStrm->NextIn = CompPtr + 12;
  IStrm->AvailIn = CompLen - 12;
  ASSERT_TRUE(Inflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  EXPECT_EQ(
      0, std::memcmp(MemInst.getPointer<char *>(DecPtr), Payload, PayloadLen));
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
}

TEST(WasmEdgeZlibTest, GZHeaderNullFieldPointersMapToZNull) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetHeader, "deflateSetHeader")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")
  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetHeader, "inflateGetHeader")
  GET_ZLIB_FUNC(Inflate, "inflate")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  fillMemContent(MemInst, 0, 64, 0xAB);

  const uint32_t DZS = 0x100;
  const uint32_t DHdr = 0x200;
  const uint32_t ExtraSrc = 0x300;
  const uint32_t NameSrc = 0x340;
  const uint32_t CommSrc = 0x380;
  const uint32_t DataPtr = 0x500;
  const uint32_t CompPtr = 0x1000;
  const uint32_t CompCap = 0x2000;
  const char *const Payload = "null-field-pointer-payload";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::fill_n(MemInst.getPointer<uint8_t *>(DZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(DHdr), sizeof(WasmGZHeader), 0);
  std::memcpy(MemInst.getPointer<uint8_t *>(ExtraSrc), "abcd", 4);
  std::strcpy(MemInst.getPointer<char *>(NameSrc), "gz-name");
  std::strcpy(MemInst.getPointer<char *>(CommSrc), "gz-comment");
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);

  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *DHeader = MemInst.getPointer<WasmGZHeader *>(DHdr);
  DHeader->Extra = ExtraSrc;
  DHeader->ExtraLen = 4;
  DHeader->Name = NameSrc;
  DHeader->Comment = CommSrc;
  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, DHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *DStrm = MemInst.getPointer<WasmZStream *>(DZS);
  DStrm->NextIn = DataPtr;
  DStrm->AvailIn = PayloadLen;
  DStrm->NextOut = CompPtr;
  DStrm->AvailOut = CompCap;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  const uint32_t CompLen = CompCap - DStrm->AvailOut;
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS}, RetVal));

  const uint32_t IZS = 0x4000;
  const uint32_t IHdr = 0x4100;
  const uint32_t DecPtr = 0x5000;
  const uint32_t DecCap = 0x1000;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(IHdr), sizeof(WasmGZHeader), 0);

  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IHeader = MemInst.getPointer<WasmGZHeader *>(IHdr);
  IHeader->Extra = 0;
  IHeader->ExtraMax = 64;
  IHeader->Name = 0;
  IHeader->NameMax = 64;
  IHeader->Comment = 0;
  IHeader->CommMax = 64;
  ASSERT_TRUE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, IHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IStrm = MemInst.getPointer<WasmZStream *>(IZS);
  IStrm->NextIn = CompPtr;
  IStrm->AvailIn = CompLen;
  IStrm->NextOut = DecPtr;
  IStrm->AvailOut = DecCap;
  ASSERT_TRUE(Inflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);

  EXPECT_EQ(IHeader->Done, 1);
  EXPECT_EQ(IHeader->ExtraLen, 4U);
  EXPECT_EQ(IHeader->Extra, 0U);
  EXPECT_EQ(IHeader->Name, 0U);
  EXPECT_EQ(IHeader->Comment, 0U);

  const std::vector<uint8_t> Expected(64, 0xAB);
  EXPECT_EQ(0,
            std::memcmp(MemInst.getPointer<uint8_t *>(0), Expected.data(), 64));

  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
}

TEST(WasmEdgeZlibTest, DeflateSetHeaderNullRevertsToDefaultHeader) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetHeader, "deflateSetHeader")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")

  const uint32_t DZS = 0x100;
  const uint32_t DHdr = 0x200;
  const uint32_t NamePtr = 0x300;
  const uint32_t DataPtr = 0x500;
  const uint32_t CompPtr = 0x1000;
  const uint32_t CompCap = 0x2000;
  const char *const Payload = "default-header-payload";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::fill_n(MemInst.getPointer<uint8_t *>(DZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(DHdr), sizeof(WasmGZHeader), 0);
  std::strcpy(MemInst.getPointer<char *>(NamePtr), "hdr-name");
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);
  fillMemContent(MemInst, 0, sizeof(WasmGZHeader), 0xFF);

  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *DHeader = MemInst.getPointer<WasmGZHeader *>(DHdr);
  DHeader->Name = NamePtr;
  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, DHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  const auto &HeaderMap = ZlibMod->getEnv().GZHeaderMap;
  EXPECT_EQ(HeaderMap.count(DZS), 1U);

  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, UINT32_C(0)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_EQ(HeaderMap.count(DZS), 0U);

  auto *DStrm = MemInst.getPointer<WasmZStream *>(DZS);
  DStrm->NextIn = DataPtr;
  DStrm->AvailIn = PayloadLen;
  DStrm->NextOut = CompPtr;
  DStrm->AvailOut = CompCap;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS}, RetVal));
}

TEST(WasmEdgeZlibTest, HardeningInflateHeaderExtraLenIsZlibOwned) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetHeader, "deflateSetHeader")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")
  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetHeader, "inflateGetHeader")
  GET_ZLIB_FUNC(Inflate, "inflate")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t DZS = 0x100;
  const uint32_t DHdr = 0x200;
  const uint32_t ExtraSrc = 0x300;
  const uint32_t DataPtr = 0x500;
  const uint32_t CompPtr = 0x1000;
  const uint32_t CompCap = 0x2000;
  const uint32_t ExtraFieldLen = 100;
  const char *const Payload = "payload-to-compress-through-gzip";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::fill_n(MemInst.getPointer<uint8_t *>(DZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(DHdr), sizeof(WasmGZHeader), 0);
  for (uint32_t I = 0; I < ExtraFieldLen; ++I) {
    *MemInst.getPointer<uint8_t *>(ExtraSrc + I) =
        static_cast<uint8_t>((I % 251) + 1);
  }
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);

  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *DHeader = MemInst.getPointer<WasmGZHeader *>(DHdr);
  DHeader->Extra = ExtraSrc;
  DHeader->ExtraLen = ExtraFieldLen;
  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, DHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *DStrm = MemInst.getPointer<WasmZStream *>(DZS);
  DStrm->NextIn = DataPtr;
  DStrm->AvailIn = PayloadLen;
  DStrm->NextOut = CompPtr;
  DStrm->AvailOut = CompCap;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  const uint32_t CompLen = CompCap - DStrm->AvailOut;
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS}, RetVal));

  const uint32_t IZS = 0x4000;
  const uint32_t IHdr = 0x4100;
  const uint32_t ExtraDst = 0x4200;
  const uint32_t ExtraMax = 200;
  const uint32_t DecPtr = 0x5000;
  const uint32_t DecCap = 0x1000;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(IHdr), sizeof(WasmGZHeader), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(ExtraDst), ExtraMax, 0);

  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IHeader = MemInst.getPointer<WasmGZHeader *>(IHdr);
  IHeader->Extra = ExtraDst;
  IHeader->ExtraMax = ExtraMax;
  ASSERT_TRUE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, IHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IStrm = MemInst.getPointer<WasmZStream *>(IZS);
  IStrm->NextIn = CompPtr;
  IStrm->AvailIn = 12;
  IStrm->NextOut = DecPtr;
  IStrm->AvailOut = DecCap;
  ASSERT_TRUE(Inflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(Z_NO_FLUSH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  IHeader->ExtraLen = 150;

  IStrm->AvailIn = CompLen - 12;
  ASSERT_TRUE(Inflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));

  EXPECT_EQ(IHeader->ExtraLen, ExtraFieldLen);
  EXPECT_EQ(*MemInst.getPointer<uint8_t *>(ExtraDst), static_cast<uint8_t>(1));
}

TEST(WasmEdgeZlibTest, HardeningInflateHeaderZeroCapacityKeepsGuestPointers) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetHeader, "deflateSetHeader")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")
  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetHeader, "inflateGetHeader")
  GET_ZLIB_FUNC(Inflate, "inflate")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t DZS = 0x100;
  const uint32_t DHdr = 0x200;
  const uint32_t ExtraSrc = 0x300;
  const uint32_t NameSrc = 0x380;
  const uint32_t CommSrc = 0x400;
  const uint32_t DataPtr = 0x500;
  const uint32_t CompPtr = 0x1000;
  const uint32_t CompCap = 0x2000;
  const uint32_t ExtraFieldLen = 8;
  const char *const Payload = "zero-capacity header fields";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::fill_n(MemInst.getPointer<uint8_t *>(DZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(DHdr), sizeof(WasmGZHeader), 0);
  for (uint32_t I = 0; I < ExtraFieldLen; ++I) {
    *MemInst.getPointer<uint8_t *>(ExtraSrc + I) = static_cast<uint8_t>(I + 1);
  }
  std::strcpy(MemInst.getPointer<char *>(NameSrc), "stream-name");
  std::strcpy(MemInst.getPointer<char *>(CommSrc), "stream-comment");
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);

  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *DHeader = MemInst.getPointer<WasmGZHeader *>(DHdr);
  DHeader->Extra = ExtraSrc;
  DHeader->ExtraLen = ExtraFieldLen;
  DHeader->Name = NameSrc;
  DHeader->Comment = CommSrc;
  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, DHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *DStrm = MemInst.getPointer<WasmZStream *>(DZS);
  DStrm->NextIn = DataPtr;
  DStrm->AvailIn = PayloadLen;
  DStrm->NextOut = CompPtr;
  DStrm->AvailOut = CompCap;
  ASSERT_TRUE(Deflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  const uint32_t CompLen = CompCap - DStrm->AvailOut;
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS}, RetVal));

  const uint32_t IZS = 0x4000;
  const uint32_t IHdr = 0x4100;
  const uint32_t OOBExtra = 0xFFFF0000;
  const uint32_t OOBName = 0xFFFF0100;
  const uint32_t OOBComment = 0xFFFF0200;
  const uint32_t DecPtr = 0x5000;
  const uint32_t DecCap = 0x1000;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(IHdr), sizeof(WasmGZHeader), 0);

  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  auto *IHeader = MemInst.getPointer<WasmGZHeader *>(IHdr);
  IHeader->Extra = OOBExtra;
  IHeader->ExtraMax = 0;
  IHeader->Name = OOBName;
  IHeader->NameMax = 0;
  IHeader->Comment = OOBComment;
  IHeader->CommMax = 0;
  ASSERT_TRUE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, IHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IStrm = MemInst.getPointer<WasmZStream *>(IZS);
  IStrm->NextIn = CompPtr;
  IStrm->AvailIn = CompLen;
  IStrm->NextOut = DecPtr;
  IStrm->AvailOut = DecCap;
  ASSERT_TRUE(Inflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);

  EXPECT_EQ(IHeader->Extra, OOBExtra);
  EXPECT_EQ(IHeader->Name, OOBName);
  EXPECT_EQ(IHeader->Comment, OOBComment);
  EXPECT_EQ(IHeader->ExtraLen, ExtraFieldLen);
  EXPECT_EQ(IHeader->Done, INT32_C(1));

  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
}

TEST(WasmEdgeZlibTest, HardeningReinitRejectsLiveStream) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit, "deflateInit")

  const uint32_t ZS = 0;
  std::fill_n(MemInst.getPointer<uint8_t *>(ZS), sizeof(WasmZStream), 0);
  ASSERT_TRUE(DeflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-1)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  ASSERT_TRUE(DeflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-1)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
}

TEST(WasmEdgeZlibTest, HardeningWrongTypeEndKeepsStream) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit, "deflateInit")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")

  const uint32_t ZS = 0;
  std::fill_n(MemInst.getPointer<uint8_t *>(ZS), sizeof(WasmZStream), 0);
  ASSERT_TRUE(DeflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-1)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
}

TEST(WasmEdgeZlibTest, HardeningWrongKindStreamAnswersLikeZlib) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit, "deflateInit")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateParams, "deflateParams")
  GET_ZLIB_FUNC(DeflateTune, "deflateTune")
  GET_ZLIB_FUNC(DeflatePending, "deflatePending")
  GET_ZLIB_FUNC(DeflatePrime, "deflatePrime")
  GET_ZLIB_FUNC(DeflateSetHeader, "deflateSetHeader")
  GET_ZLIB_FUNC(DeflateGetDictionary, "deflateGetDictionary")
  GET_ZLIB_FUNC(DeflateReset, "deflateReset")
  GET_ZLIB_FUNC(DeflateResetKeep, "deflateResetKeep")
  GET_ZLIB_FUNC(DeflateCopy, "deflateCopy")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")
  GET_ZLIB_FUNC(InflateInit, "inflateInit")
  GET_ZLIB_FUNC(Inflate, "inflate")
  GET_ZLIB_FUNC(InflateSync, "inflateSync")
  GET_ZLIB_FUNC(InflateMark, "inflateMark")
  GET_ZLIB_FUNC(InflateGetDictionary, "inflateGetDictionary")
  GET_ZLIB_FUNC(InflateValidate, "inflateValidate")
  GET_ZLIB_FUNC(InflateCodesUsed, "inflateCodesUsed")
  GET_ZLIB_FUNC(InflateSyncPoint, "inflateSyncPoint")
  GET_ZLIB_FUNC(InflateUndermine, "inflateUndermine")
  GET_ZLIB_FUNC(InflatePrime, "inflatePrime")
  GET_ZLIB_FUNC(InflateReset, "inflateReset")
  GET_ZLIB_FUNC(InflateReset2, "inflateReset2")
  GET_ZLIB_FUNC(InflateResetKeep, "inflateResetKeep")
  GET_ZLIB_FUNC(InflateCopy, "inflateCopy")
  GET_ZLIB_FUNC(InflateBackEnd, "inflateBackEnd")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t OOBPtr = 0xFFFF0000;
  const uint32_t DZS = 0x40;
  const uint32_t IZS = 0x100;
  fillMemContent(MemInst, DZS, sizeof(WasmZStream));
  fillMemContent(MemInst, IZS, sizeof(WasmZStream));
  ASSERT_TRUE(DeflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, INT32_C(-1)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(InflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  for (const uint32_t ZS : {DZS, IZS}) {
    auto *Strm = MemInst.getPointer<WasmZStream *>(ZS);
    Strm->NextIn = OOBPtr;
    Strm->AvailIn = 0x1000;
    Strm->NextOut = OOBPtr;
    Strm->AvailOut = 0x1000;
  }

  const auto ExpectI32 = [&](auto &Func,
                             std::initializer_list<WasmEdge::ValVariant> Args,
                             int32_t Expected) {
    EXPECT_TRUE(Func.run(CallFrame, Args, RetVal));
    EXPECT_EQ(RetVal[0].get<int32_t>(), Expected);
  };

  ExpectI32(Deflate, {IZS, INT32_C(Z_NO_FLUSH)}, Z_STREAM_ERROR);
  ExpectI32(DeflateParams, {IZS, INT32_C(9), INT32_C(Z_DEFAULT_STRATEGY)},
            Z_STREAM_ERROR);
  ExpectI32(DeflateTune,
            {IZS, INT32_C(8), INT32_C(16), INT32_C(32), INT32_C(128)},
            Z_STREAM_ERROR);
  ExpectI32(DeflatePending, {IZS, OOBPtr, OOBPtr}, Z_STREAM_ERROR);
  ExpectI32(DeflatePrime, {IZS, INT32_C(5), INT32_C(21)}, Z_STREAM_ERROR);
  ExpectI32(DeflateSetHeader, {IZS, UINT32_C(0x300)}, Z_STREAM_ERROR);
  ExpectI32(DeflateGetDictionary, {IZS, OOBPtr, OOBPtr}, Z_STREAM_ERROR);
  ExpectI32(DeflateReset, {IZS}, Z_STREAM_ERROR);
  ExpectI32(DeflateResetKeep, {IZS}, Z_STREAM_ERROR);
  ExpectI32(DeflateEnd, {IZS}, Z_STREAM_ERROR);

  ExpectI32(Inflate, {DZS, INT32_C(Z_NO_FLUSH)}, Z_STREAM_ERROR);
  ExpectI32(InflateSync, {DZS}, Z_STREAM_ERROR);
  ExpectI32(InflateMark, {DZS}, INT32_C(-65536));
  ExpectI32(InflateGetDictionary, {DZS, OOBPtr, OOBPtr}, Z_STREAM_ERROR);
  ExpectI32(InflateValidate, {DZS, INT32_C(1)}, Z_STREAM_ERROR);
  ExpectI32(InflateCodesUsed, {DZS}, INT32_C(-1));
  ExpectI32(InflateSyncPoint, {DZS}, Z_STREAM_ERROR);
  ExpectI32(InflateUndermine, {DZS, INT32_C(1)}, Z_STREAM_ERROR);
  ExpectI32(InflatePrime, {DZS, INT32_C(5), INT32_C(21)}, Z_STREAM_ERROR);
  ExpectI32(InflateReset, {DZS}, Z_STREAM_ERROR);
  ExpectI32(InflateReset2, {DZS, INT32_C(-15)}, Z_STREAM_ERROR);
  ExpectI32(InflateResetKeep, {DZS}, Z_STREAM_ERROR);
  ExpectI32(InflateBackEnd, {DZS}, Z_STREAM_ERROR);

  const uint32_t CopyPtr = 0x200;
  fillMemContent(MemInst, CopyPtr, sizeof(WasmZStream));
  ExpectI32(InflateCopy, {CopyPtr, DZS}, Z_STREAM_ERROR);
  ExpectI32(DeflateCopy, {CopyPtr, IZS}, Z_STREAM_ERROR);
  ExpectI32(DeflateInit, {CopyPtr, INT32_C(-1)}, Z_OK);
  ExpectI32(DeflateEnd, {CopyPtr}, Z_OK);

  const uint32_t DataPtr = 0x300;
  const char *const Payload = "wrong-kind guard leaves streams intact";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);
  auto *DStrm = MemInst.getPointer<WasmZStream *>(DZS);
  DStrm->NextIn = DataPtr;
  DStrm->AvailIn = PayloadLen;
  DStrm->NextOut = 0x1000;
  DStrm->AvailOut = 0x1000;
  ExpectI32(Deflate, {DZS, INT32_C(Z_FINISH)}, Z_STREAM_END);
  ExpectI32(DeflateEnd, {DZS}, Z_OK);
  ExpectI32(InflateEnd, {IZS}, Z_OK);
}

TEST(WasmEdgeZlibTest, HardeningDeflateCopyRejectsLiveDest) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit, "deflateInit")
  GET_ZLIB_FUNC(DeflateCopy, "deflateCopy")

  const uint32_t Dest = 0;
  const uint32_t Source = 0x100;
  std::fill_n(MemInst.getPointer<uint8_t *>(Dest), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(Source), sizeof(WasmZStream), 0);
  ASSERT_TRUE(DeflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{Dest, INT32_C(-1)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(DeflateInit.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{Source, INT32_C(-1)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  ASSERT_TRUE(DeflateCopy.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{Dest, Source},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
}

TEST(WasmEdgeZlibTest, HardeningFailedInitLeavesKeyReusable) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 2)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit, "deflateInit")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")

  const uint32_t ZS = UINT32_C(65536);
  ASSERT_FALSE(DeflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-1)},
      RetVal));

  ASSERT_TRUE(MemInst.growPage(1));
  std::fill_n(MemInst.getPointer<uint8_t *>(ZS), sizeof(WasmZStream), 0);
  ASSERT_TRUE(DeflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-1)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
}

TEST(WasmEdgeZlibTest, HardeningFailedCopyLeavesKeyReusable) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 2)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit, "deflateInit")
  GET_ZLIB_FUNC(DeflateCopy, "deflateCopy")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")
  GET_ZLIB_FUNC(InflateInit, "inflateInit")
  GET_ZLIB_FUNC(InflateCopy, "inflateCopy")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t DeflateSrc = 0;
  const uint32_t InflateSrc = 0x100;
  const uint32_t DeflateDest = UINT32_C(65536);
  const uint32_t InflateDest = UINT32_C(65536) + 0x100;
  std::fill_n(MemInst.getPointer<uint8_t *>(DeflateSrc), sizeof(WasmZStream),
              0);
  std::fill_n(MemInst.getPointer<uint8_t *>(InflateSrc), sizeof(WasmZStream),
              0);
  ASSERT_TRUE(DeflateInit.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DeflateSrc, INT32_C(-1)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(InflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{InflateSrc},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  ASSERT_FALSE(DeflateCopy.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DeflateDest, DeflateSrc},
      RetVal));
  ASSERT_FALSE(InflateCopy.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{InflateDest, InflateSrc},
      RetVal));

  ASSERT_TRUE(MemInst.growPage(1));
  std::fill_n(MemInst.getPointer<uint8_t *>(DeflateDest), sizeof(WasmZStream),
              0);
  std::fill_n(MemInst.getPointer<uint8_t *>(InflateDest), sizeof(WasmZStream),
              0);
  ASSERT_TRUE(DeflateCopy.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DeflateDest, DeflateSrc},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(InflateCopy.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{InflateDest, InflateSrc},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  for (const uint32_t ZS : {DeflateSrc, DeflateDest}) {
    ASSERT_TRUE(DeflateEnd.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
    EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  }
  for (const uint32_t ZS : {InflateSrc, InflateDest}) {
    ASSERT_TRUE(InflateEnd.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
    EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  }
}

TEST(WasmEdgeZlibTest, HardeningDeflateSetHeaderClampsExtra) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(2, 2)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetHeader, "deflateSetHeader")

  const uint32_t DZS = 0;
  const uint32_t DHdr = 0x100;
  std::fill_n(MemInst.getPointer<uint8_t *>(DZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(DHdr), sizeof(WasmGZHeader), 0);
  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  const uint32_t MemBytes = static_cast<uint32_t>(MemInst.getSize());
  const uint32_t ExtraOff = MemBytes - 8;
  for (uint32_t I = 0; I < 8; ++I) {
    *MemInst.getPointer<uint8_t *>(ExtraOff + I) = static_cast<uint8_t>(I + 1);
  }
  auto *DHeader = MemInst.getPointer<WasmGZHeader *>(DHdr);
  DHeader->Extra = ExtraOff;
  DHeader->ExtraLen = UINT32_C(0x10008);

  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, DHdr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
}

TEST(WasmEdgeZlibTest, DeflateSetHeaderKeepsEmptyExtraField) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(2, 2)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetHeader, "deflateSetHeader")
  GET_ZLIB_FUNC(Deflate, "deflate")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")

  const std::array<uint32_t, 2> ZeroEmittedLens = {UINT32_C(0),
                                                   UINT32_C(0x10000)};
  for (const uint32_t ExtraLen : ZeroEmittedLens) {
    const uint32_t DZS = 0x100;
    const uint32_t DHdr = 0x200;
    const uint32_t ExtraPtr = 0x300;
    const uint32_t DataPtr = 0x500;
    const uint32_t CompPtr = 0x1000;
    const uint32_t CompCap = 0x1000;
    const char *const Payload = "payload";
    std::fill_n(MemInst.getPointer<uint8_t *>(DZS), sizeof(WasmZStream), 0);
    std::fill_n(MemInst.getPointer<uint8_t *>(DHdr), sizeof(WasmGZHeader), 0);
    std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);

    ASSERT_TRUE(
        DeflateInit2.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(31),
                             INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                         RetVal));
    ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

    auto *DHeader = MemInst.getPointer<WasmGZHeader *>(DHdr);
    DHeader->Extra = ExtraPtr;
    DHeader->ExtraLen = ExtraLen;
    ASSERT_TRUE(DeflateSetHeader.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, DHdr},
        RetVal));
    ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

    auto *DStrm = MemInst.getPointer<WasmZStream *>(DZS);
    DStrm->NextIn = DataPtr;
    DStrm->AvailIn = static_cast<uint32_t>(std::strlen(Payload));
    DStrm->NextOut = CompPtr;
    DStrm->AvailOut = CompCap;
    ASSERT_TRUE(Deflate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{DZS, INT32_C(Z_FINISH)},
        RetVal));
    ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
    ASSERT_TRUE(DeflateEnd.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS}, RetVal));

    const auto *Out = MemInst.getPointer<const uint8_t *>(CompPtr);
    EXPECT_NE(Out[3] & 0x04, 0) << "FEXTRA dropped for ExtraLen " << ExtraLen;
    EXPECT_EQ(Out[10], 0) << "XLEN low byte for ExtraLen " << ExtraLen;
    EXPECT_EQ(Out[11], 0) << "XLEN high byte for ExtraLen " << ExtraLen;
  }
}

TEST(WasmEdgeZlibTest, HardeningGZBufferClampsSize) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  const std::string TmpPath = (std::filesystem::temp_directory_path() /
                               "wasmedge_zlib_gzbuffer_test.gz")
                                  .string();
  ASSERT_LT(TmpPath.size() + 1, 1000U);
  const uint32_t PathPtr = 16;
  std::strcpy(MemInst.getPointer<char *>(PathPtr), TmpPath.c_str());
  const uint32_t ModeWPtr = 1024;
  std::strcpy(MemInst.getPointer<char *>(ModeWPtr), "wb");

  GET_ZLIB_FUNC(GZOpen, "gzopen")
  GET_ZLIB_FUNC(GZBuffer, "gzbuffer")
  GET_ZLIB_FUNC(GZClose, "gzclose")

  if (!GZOpen.run(
          CallFrame,
          std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeWPtr},
          RetVal)) {
    GTEST_SKIP() << "cannot create temporary file: " << TmpPath;
  }
  const uint32_t WHandle = RetVal[0].get<uint32_t>();

  EXPECT_TRUE(GZBuffer.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               WHandle, UINT32_C(0xFFFFFFFF)},
                           RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), -1);

  EXPECT_TRUE(GZBuffer.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               WHandle, UINT32_C(0x40000000)},
                           RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 0);

  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{WHandle}, RetVal));
  std::remove(TmpPath.c_str());
}

TEST(WasmEdgeZlibTest, HardeningDeflatePendingRejectsOOBPointer) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit, "deflateInit")
  GET_ZLIB_FUNC(DeflatePending, "deflatePending")

  const uint32_t ZS = 0;
  std::fill_n(MemInst.getPointer<uint8_t *>(ZS), sizeof(WasmZStream), 0);
  ASSERT_TRUE(DeflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-1)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  EXPECT_FALSE(DeflatePending.run(CallFrame,
                                  std::initializer_list<WasmEdge::ValVariant>{
                                      ZS, UINT32_C(65534), UINT32_C(0x100)},
                                  RetVal));
}

TEST(WasmEdgeZlibTest, InflateGetHeaderSyncsImmediateDoneReset) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetHeader, "inflateGetHeader")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  const uint32_t IZS = 0x100;
  const uint32_t IHdr = 0x200;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  std::fill_n(MemInst.getPointer<uint8_t *>(IHdr), sizeof(WasmGZHeader), 0);

  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(31)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  auto *IHeader = MemInst.getPointer<WasmGZHeader *>(IHdr);
  IHeader->Done = 1;
  ASSERT_TRUE(InflateGetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, IHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_EQ(IHeader->Done, 0);

  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
}

TEST(WasmEdgeZlibTest, GetDictionaryAndPendingHonorNullOutPointers) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(16, 16)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit2, "deflateInit2")
  GET_ZLIB_FUNC(DeflateSetDictionary, "deflateSetDictionary")
  GET_ZLIB_FUNC(DeflateGetDictionary, "deflateGetDictionary")
  GET_ZLIB_FUNC(DeflatePending, "deflatePending")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")
  GET_ZLIB_FUNC(InflateInit2, "inflateInit2")
  GET_ZLIB_FUNC(InflateGetDictionary, "inflateGetDictionary")
  GET_ZLIB_FUNC(InflateEnd, "inflateEnd")

  fillMemContent(MemInst, 0, 64, 0xCD);
  const std::vector<uint8_t> Expected(64, 0xCD);

  const uint32_t DZS = 0x100;
  const uint32_t DictSrc = 0x200;
  const uint32_t DictOut = 0x240;
  const uint32_t LenPtr = 0x280;
  const uint32_t BitsPtr = 0x290;
  const uint32_t PendPtr = 0x2A0;
  const char *const Dict = "0123456789abcdef";
  const uint32_t DictLen = 16;
  std::memcpy(MemInst.getPointer<uint8_t *>(DictSrc), Dict, DictLen);
  std::fill_n(MemInst.getPointer<uint8_t *>(DZS), sizeof(WasmZStream), 0);

  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(15),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(DeflateSetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, DictSrc, DictLen},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  ASSERT_TRUE(DeflateGetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, UINT32_C(0), LenPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_EQ(*MemInst.getPointer<uint32_t *>(LenPtr), DictLen);

  ASSERT_TRUE(DeflateGetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, DictOut, UINT32_C(0)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_EQ(0,
            std::memcmp(MemInst.getPointer<uint8_t *>(DictOut), Dict, DictLen));

  ASSERT_TRUE(DeflatePending.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, UINT32_C(0), BitsPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(DeflatePending.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DZS, PendPtr, UINT32_C(0)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  EXPECT_EQ(0,
            std::memcmp(MemInst.getPointer<uint8_t *>(0), Expected.data(), 64));
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS}, RetVal));

  const uint32_t IZS = 0x400;
  std::fill_n(MemInst.getPointer<uint8_t *>(IZS), sizeof(WasmZStream), 0);
  ASSERT_TRUE(InflateInit2.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(15)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(InflateGetDictionary.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{IZS, DictOut, UINT32_C(0)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  EXPECT_EQ(0,
            std::memcmp(MemInst.getPointer<uint8_t *>(0), Expected.data(), 64));
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
}

TEST(WasmEdgeZlibTest, HardeningInflateBackInitVersionErrorSkipsWindow) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(InflateBackInit_, "inflateBackInit_")

  const uint32_t ZS = 0;
  std::fill_n(MemInst.getPointer<uint8_t *>(ZS), sizeof(WasmZStream), 0);
  const uint32_t OOBWindowPtr = 0xFFFF0000;

  const uint32_t BadVersionPtr = 0x100;
  std::strcpy(MemInst.getPointer<char *>(BadVersionPtr), "0.0.0");
  ASSERT_TRUE(
      InflateBackInit_.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               ZS, INT32_C(15), OOBWindowPtr, BadVersionPtr,
                               static_cast<int32_t>(sizeof(WasmZStream))},
                           RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);

  const uint32_t VersionPtr = 0x140;
  std::strcpy(MemInst.getPointer<char *>(VersionPtr), ZLIB_VERSION);
  EXPECT_FALSE(
      InflateBackInit_.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               ZS, INT32_C(15), OOBWindowPtr, VersionPtr,
                               static_cast<int32_t>(sizeof(WasmZStream))},
                           RetVal));
}

TEST(WasmEdgeZlibTest, HardeningInflateBackInitNullWindowMatchesZlib) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(InflateBackInit, "inflateBackInit")
  GET_ZLIB_FUNC(InflateBackEnd, "inflateBackEnd")

  const uint32_t ZS = 0;
  fillMemContent(MemInst, ZS, sizeof(WasmZStream));
  ASSERT_TRUE(InflateBackInit.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(15), UINT32_C(0)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);

  ASSERT_TRUE(InflateBackInit.run(CallFrame,
                                  std::initializer_list<WasmEdge::ValVariant>{
                                      ZS, INT32_C(15), UINT32_C(0x1000)},
                                  RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(InflateBackEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
}

TEST(WasmEdgeZlibTest, HardeningVersionedInitAnswersVersionErrorFirst) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit_, "deflateInit_")
  GET_ZLIB_FUNC(InflateInit_, "inflateInit_")
  GET_ZLIB_FUNC(DeflateInit2_, "deflateInit2_")
  GET_ZLIB_FUNC(InflateInit2_, "inflateInit2_")
  GET_ZLIB_FUNC(DeflateEnd, "deflateEnd")

  const int32_t StreamSize = static_cast<int32_t>(sizeof(WasmZStream));
  const uint32_t BadVersionPtr = 0x100;
  std::strcpy(MemInst.getPointer<char *>(BadVersionPtr), "0.0.0");
  const uint32_t OOBZStreamPtr = 0xFFFF0000;

  ASSERT_TRUE(DeflateInit_.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{OOBZStreamPtr, INT32_C(-1),
                                                  BadVersionPtr, StreamSize},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);
  ASSERT_TRUE(InflateInit_.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   OOBZStreamPtr, BadVersionPtr, StreamSize},
                               RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);
  ASSERT_TRUE(DeflateInit2_.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          OOBZStreamPtr, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(15),
          INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY), BadVersionPtr, StreamSize},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);
  ASSERT_TRUE(InflateInit2_.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{OOBZStreamPtr, INT32_C(15),
                                                  BadVersionPtr, StreamSize},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);

  const uint32_t ZS = 0;
  fillMemContent(MemInst, ZS, sizeof(WasmZStream));
  const uint32_t VersionPtr = 0x140;
  std::strcpy(MemInst.getPointer<char *>(VersionPtr), ZLIB_VERSION);
  ASSERT_TRUE(DeflateInit_.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ZS, INT32_C(-1), VersionPtr, StreamSize},
                               RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);
  ASSERT_TRUE(DeflateInit_.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ZS, INT32_C(-1), BadVersionPtr, StreamSize},
                               RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);
  ASSERT_TRUE(DeflateInit_.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ZS, INT32_C(-1), VersionPtr, StreamSize},
                               RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
}

TEST(WasmEdgeZlibTest, HardeningDeflateInitValidatesVersionString) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  GET_ZLIB_FUNC(DeflateInit_, "deflateInit_")

  const uint32_t ZS = 0;
  std::fill_n(MemInst.getPointer<uint8_t *>(ZS), sizeof(WasmZStream), 0);

  const uint32_t MemBytes = static_cast<uint32_t>(MemInst.getSize());
  const uint32_t VersionPtr = MemBytes - 4;
  std::fill_n(MemInst.getPointer<uint8_t *>(VersionPtr), 4,
              static_cast<uint8_t>(ZLIB_VERSION[0]));

  ASSERT_TRUE(DeflateInit_.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ZS, INT32_C(-1), VersionPtr,
                                   static_cast<int32_t>(sizeof(WasmZStream))},
                               RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_VERSION_ERROR);
}

TEST(WasmEdgeZlibTest, HardeningGZOpenMediatedByWasi) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasiWiredModule Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;

  const std::filesystem::path SandboxDir =
      std::filesystem::temp_directory_path() / "wasmedge_zlib_wasi_sandbox";
  std::error_code EC;
  std::filesystem::create_directories(SandboxDir, EC);
  std::filesystem::remove(SandboxDir / "wasi.gz", EC);

  WasmEdge::Host::WasiModule WasiMod;
  WasiMod.init(std::vector<std::string>{SandboxDir.string()}, "test", {}, {});
  Mod.wireWASIModule(&WasiMod);
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  const uint32_t PathPtr = 16;
  std::strcpy(MemInst.getPointer<char *>(PathPtr), "wasi.gz");
  const uint32_t ModeWPtr = 64;
  std::strcpy(MemInst.getPointer<char *>(ModeWPtr), "wb");
  const uint32_t ModeRPtr = 96;
  std::strcpy(MemInst.getPointer<char *>(ModeRPtr), "rb");
  const uint32_t EscapePtr = 128;
  std::strcpy(MemInst.getPointer<char *>(EscapePtr), "../wasi_escape.gz");
  const uint32_t DataPtr = 256;
  const char *const Payload = "hello wasi-mediated gzip";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);

  GET_ZLIB_FUNC(GZOpen, "gzopen")
  GET_ZLIB_FUNC(GZWrite, "gzwrite")
  GET_ZLIB_FUNC(GZRead, "gzread")
  GET_ZLIB_FUNC(GZClose, "gzclose")

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeWPtr},
      RetVal));
  const uint32_t WHandle = RetVal[0].get<uint32_t>();
  if (WHandle == 0) {
    GTEST_SKIP() << "WASI preopen unavailable in this environment";
  }
  EXPECT_TRUE(GZWrite.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{WHandle, DataPtr, PayloadLen},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{WHandle}, RetVal));

  EXPECT_TRUE(std::filesystem::exists(SandboxDir / "wasi.gz"));

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeRPtr},
      RetVal));
  const uint32_t RHandle = RetVal[0].get<uint32_t>();
  ASSERT_NE(RHandle, 0U);
  const uint32_t ReadBuf = 512;
  EXPECT_TRUE(GZRead.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             RHandle, ReadBuf, UINT32_C(256)},
                         RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
  EXPECT_EQ(
      0, std::memcmp(MemInst.getPointer<char *>(ReadBuf), Payload, PayloadLen));
  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{RHandle}, RetVal));

  EXPECT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{EscapePtr, ModeWPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);

  std::filesystem::remove(SandboxDir / "wasi.gz", EC);
  std::filesystem::remove(SandboxDir / "wasi_escape.gz", EC);
}

TEST(WasmEdgeZlibTest, HardeningGZOpenWasiAcceptsAbsoluteGuestPaths) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasiWiredModule Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;

  const std::filesystem::path SandboxDir =
      std::filesystem::temp_directory_path() / "wasmedge_zlib_wasi_abs";
  std::error_code EC;
  std::filesystem::create_directories(SandboxDir, EC);
  std::filesystem::remove(SandboxDir / "wasi_probe.gz", EC);
  std::filesystem::remove(SandboxDir / "wasi_abs.gz", EC);

  WasmEdge::Host::WasiModule WasiMod;
  WasiMod.init(std::vector<std::string>{SandboxDir.string()}, "test", {}, {});
  Mod.wireWASIModule(&WasiMod);
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  const uint32_t ProbePtr = 16;
  std::strcpy(MemInst.getPointer<char *>(ProbePtr), "wasi_probe.gz");
  const uint32_t AbsPtr = 96;
  std::strcpy(MemInst.getPointer<char *>(AbsPtr), "/wasi_abs.gz");
  const uint32_t DotPtr = 176;
  std::strcpy(MemInst.getPointer<char *>(DotPtr), "./wasi_abs.gz");
  const uint32_t MixedPtr = 256;
  std::strcpy(MemInst.getPointer<char *>(MixedPtr), "//./wasi_abs.gz");
  const uint32_t EscapePtr = 336;
  std::strcpy(MemInst.getPointer<char *>(EscapePtr), "/../wasi_abs_escape.gz");
  const uint32_t ModeWPtr = 512;
  std::strcpy(MemInst.getPointer<char *>(ModeWPtr), "wb");
  const uint32_t ModeRPtr = 544;
  std::strcpy(MemInst.getPointer<char *>(ModeRPtr), "rb");
  const uint32_t DataPtr = 1024;
  const char *const Payload = "absolute path through the preopen";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);

  GET_ZLIB_FUNC(GZOpen, "gzopen")
  GET_ZLIB_FUNC(GZWrite, "gzwrite")
  GET_ZLIB_FUNC(GZRead, "gzread")
  GET_ZLIB_FUNC(GZClose, "gzclose")

  ASSERT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ProbePtr, ModeWPtr}, RetVal));
  const uint32_t ProbeHandle = RetVal[0].get<uint32_t>();
  if (ProbeHandle == 0) {
    GTEST_SKIP() << "WASI preopen unavailable in this environment";
  }
  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ProbeHandle},
      RetVal));

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{AbsPtr, ModeWPtr},
      RetVal));
  const uint32_t WHandle = RetVal[0].get<uint32_t>();
  ASSERT_NE(WHandle, 0U);
  EXPECT_TRUE(GZWrite.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{WHandle, DataPtr, PayloadLen},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{WHandle}, RetVal));
  EXPECT_TRUE(std::filesystem::exists(SandboxDir / "wasi_abs.gz"));

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DotPtr, ModeRPtr},
      RetVal));
  const uint32_t RHandle = RetVal[0].get<uint32_t>();
  ASSERT_NE(RHandle, 0U);
  const uint32_t ReadBuf = 2048;
  EXPECT_TRUE(GZRead.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             RHandle, ReadBuf, UINT32_C(256)},
                         RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
  EXPECT_EQ(
      0, std::memcmp(MemInst.getPointer<char *>(ReadBuf), Payload, PayloadLen));
  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{RHandle}, RetVal));

  ASSERT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{MixedPtr, ModeRPtr}, RetVal));
  const uint32_t MHandle = RetVal[0].get<uint32_t>();
  ASSERT_NE(MHandle, 0U);
  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{MHandle}, RetVal));

  EXPECT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{EscapePtr, ModeWPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);

  std::filesystem::remove(SandboxDir / "wasi_probe.gz", EC);
  std::filesystem::remove(SandboxDir / "wasi_abs.gz", EC);
}

TEST(WasmEdgeZlibTest, HardeningGZOpenUnrecognizedWasiFailsClosed) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasiWiredModule Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;

  WasmEdge::Runtime::Instance::ModuleInstance StubWasi(
      "wasi_snapshot_preview1");
  Mod.wireWASIModule(&StubWasi);
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  const std::filesystem::path HostFile =
      std::filesystem::temp_directory_path() /
      "wasmedge_zlib_unrecognized_wasi.txt";
  {
    std::ofstream OFS(HostFile, std::ios::binary | std::ios::trunc);
    if (!OFS) {
      GTEST_SKIP() << "cannot create host probe file";
    }
    OFS << "not a gzip file, but gzopen reads it transparently";
  }

  const uint32_t PathPtr = 16;
  const std::string HostPathStr = HostFile.string();
  std::strcpy(MemInst.getPointer<char *>(PathPtr), HostPathStr.c_str());
  const uint32_t ModePtr = 2048;
  std::strcpy(MemInst.getPointer<char *>(ModePtr), "rb");

  GET_ZLIB_FUNC(GZOpen, "gzopen")

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModePtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);

  std::error_code EC;
  std::filesystem::remove(HostFile, EC);
}

TEST(WasmEdgeZlibTest, HardeningGZOpenWasiHonorsOpenModes) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasiWiredModule Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;

  const std::filesystem::path SandboxDir =
      std::filesystem::temp_directory_path() / "wasmedge_zlib_wasi_modes";
  std::error_code EC;
  std::filesystem::create_directories(SandboxDir, EC);
  std::filesystem::remove(SandboxDir / "modes.gz", EC);
  std::filesystem::remove(SandboxDir / "fresh.gz", EC);
  std::filesystem::remove(SandboxDir / "transparent.gz", EC);
  std::filesystem::remove(SandboxDir / "gfresh.gz", EC);

  WasmEdge::Host::WasiModule WasiMod;
  WasiMod.init(std::vector<std::string>{SandboxDir.string()}, "test", {}, {});
  Mod.wireWASIModule(&WasiMod);
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  const uint32_t PathPtr = 16;
  std::strcpy(MemInst.getPointer<char *>(PathPtr), "modes.gz");
  const uint32_t FreshPtr = 32;
  std::strcpy(MemInst.getPointer<char *>(FreshPtr), "fresh.gz");
  const uint32_t ModeWPtr = 64;
  std::strcpy(MemInst.getPointer<char *>(ModeWPtr), "wb");
  const uint32_t ModeWPlusPtr = 96;
  std::strcpy(MemInst.getPointer<char *>(ModeWPlusPtr), "w+");
  const uint32_t ModeNoDirPtr = 128;
  std::strcpy(MemInst.getPointer<char *>(ModeNoDirPtr), "b");
  const uint32_t ModeWxPtr = 160;
  std::strcpy(MemInst.getPointer<char *>(ModeWxPtr), "wx");
  const uint32_t ModeWrPtr = 192;
  std::strcpy(MemInst.getPointer<char *>(ModeWrPtr), "wr");
  const uint32_t ModeAPtr = 224;
  std::strcpy(MemInst.getPointer<char *>(ModeAPtr), "a");
  const uint32_t ModeRTPtr = 288;
  std::strcpy(MemInst.getPointer<char *>(ModeRTPtr), "rT");
  const uint32_t ModeWTPtr = 320;
  std::strcpy(MemInst.getPointer<char *>(ModeWTPtr), "wT");
  const uint32_t TransPathPtr = 352;
  std::strcpy(MemInst.getPointer<char *>(TransPathPtr), "transparent.gz");
  const uint32_t ModeWGPtr = 368;
  std::strcpy(MemInst.getPointer<char *>(ModeWGPtr), "wG");
  const uint32_t ModeAGPtr = 384;
  std::strcpy(MemInst.getPointer<char *>(ModeAGPtr), "aG");
  const uint32_t ModeWTGPtr = 400;
  std::strcpy(MemInst.getPointer<char *>(ModeWTGPtr), "wTG");
  const uint32_t ModeWGTPtr = 416;
  std::strcpy(MemInst.getPointer<char *>(ModeWGTPtr), "wGT");
  const uint32_t ModeRGPtr = 432;
  std::strcpy(MemInst.getPointer<char *>(ModeRGPtr), "rG");
  const uint32_t ModeRNPtr = 448;
  std::strcpy(MemInst.getPointer<char *>(ModeRNPtr), "rN");
  const uint32_t GFreshPtr = 464;
  std::strcpy(MemInst.getPointer<char *>(GFreshPtr), "gfresh.gz");
  const uint32_t FifoPtr = 480;
  std::strcpy(MemInst.getPointer<char *>(FifoPtr), "fifo.gz");
  const uint32_t ModeWNPtr = 496;
  std::strcpy(MemInst.getPointer<char *>(ModeWNPtr), "wN");
  const uint32_t DataPtr = 256;
  const char *const Payload = "gzopen mode fidelity";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);
  const uint32_t ReadBuf = 512;

  GET_ZLIB_FUNC(GZOpen, "gzopen")
  GET_ZLIB_FUNC(GZWrite, "gzwrite")
  GET_ZLIB_FUNC(GZRead, "gzread")
  GET_ZLIB_FUNC(GZClose, "gzclose")

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeWPtr},
      RetVal));
  const uint32_t WHandle = RetVal[0].get<uint32_t>();
  if (WHandle == 0) {
    GTEST_SKIP() << "WASI preopen unavailable in this environment";
  }
  ASSERT_TRUE(GZWrite.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{WHandle, DataPtr, PayloadLen},
      RetVal));
  ASSERT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{WHandle}, RetVal));
  const auto BaselineSize = std::filesystem::file_size(SandboxDir / "modes.gz");

  EXPECT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeWPlusPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);
  EXPECT_EQ(std::filesystem::file_size(SandboxDir / "modes.gz"), BaselineSize);

  EXPECT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeNoDirPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);
  EXPECT_EQ(std::filesystem::file_size(SandboxDir / "modes.gz"), BaselineSize);

  EXPECT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeRTPtr}, RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);
  EXPECT_EQ(std::filesystem::file_size(SandboxDir / "modes.gz"), BaselineSize);

  EXPECT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeWGPtr}, RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);
  EXPECT_EQ(std::filesystem::file_size(SandboxDir / "modes.gz"), BaselineSize);
  EXPECT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{GFreshPtr, ModeWGPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);
  EXPECT_FALSE(std::filesystem::exists(SandboxDir / "gfresh.gz"));
  EXPECT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeAGPtr}, RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);
  EXPECT_EQ(std::filesystem::file_size(SandboxDir / "modes.gz"), BaselineSize);

  EXPECT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeWTGPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);
  EXPECT_EQ(std::filesystem::file_size(SandboxDir / "modes.gz"), BaselineSize);

  ASSERT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{TransPathPtr, ModeWTPtr},
      RetVal));
  const uint32_t THandle = RetVal[0].get<uint32_t>();
  EXPECT_NE(THandle, 0U);
  if (THandle != 0) {
    EXPECT_TRUE(GZClose.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{THandle},
        RetVal));
  }

  ASSERT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{TransPathPtr, ModeWGTPtr},
      RetVal));
  const uint32_t GTHandle = RetVal[0].get<uint32_t>();
  EXPECT_NE(GTHandle, 0U);
  if (GTHandle != 0) {
    EXPECT_TRUE(GZClose.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{GTHandle},
        RetVal));
  }

  EXPECT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeWxPtr}, RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);
  EXPECT_EQ(std::filesystem::file_size(SandboxDir / "modes.gz"), BaselineSize);

  ASSERT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeWrPtr}, RetVal));
  const uint32_t RHandle = RetVal[0].get<uint32_t>();
  ASSERT_NE(RHandle, 0U);
  EXPECT_TRUE(GZRead.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             RHandle, ReadBuf, UINT32_C(256)},
                         RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
  EXPECT_EQ(
      0, std::memcmp(MemInst.getPointer<char *>(ReadBuf), Payload, PayloadLen));
  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{RHandle}, RetVal));
  EXPECT_EQ(std::filesystem::file_size(SandboxDir / "modes.gz"), BaselineSize);

  for (const uint32_t ModePtr : {ModeRGPtr, ModeRNPtr}) {
    ASSERT_TRUE(GZOpen.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModePtr}, RetVal));
    const uint32_t Handle = RetVal[0].get<uint32_t>();
    EXPECT_NE(Handle, 0U);
    if (Handle == 0) {
      continue;
    }
    EXPECT_TRUE(GZRead.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               Handle, ReadBuf, UINT32_C(256)},
                           RetVal));
    EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
    EXPECT_EQ(0, std::memcmp(MemInst.getPointer<char *>(ReadBuf), Payload,
                             PayloadLen));
    EXPECT_TRUE(GZClose.run(CallFrame,
                            std::initializer_list<WasmEdge::ValVariant>{Handle},
                            RetVal));
  }
  EXPECT_EQ(std::filesystem::file_size(SandboxDir / "modes.gz"), BaselineSize);

#if !defined(_WIN32)
  const std::filesystem::path FifoPath = SandboxDir / "fifo.gz";
  std::filesystem::remove(FifoPath, EC);
  ASSERT_EQ(::mkfifo(FifoPath.c_str(), 0600), 0);
  EXPECT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FifoPtr, ModeWNPtr}, RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);
  ASSERT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FifoPtr, ModeRNPtr}, RetVal));
  const uint32_t NHandle = RetVal[0].get<uint32_t>();
  EXPECT_NE(NHandle, 0U);
  if (NHandle != 0) {
    EXPECT_TRUE(GZClose.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{NHandle},
        RetVal));
  }
  std::filesystem::remove(FifoPath, EC);
#endif

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeAPtr},
      RetVal));
  const uint32_t AHandle = RetVal[0].get<uint32_t>();
  ASSERT_NE(AHandle, 0U);
  EXPECT_TRUE(GZWrite.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{AHandle, DataPtr, PayloadLen},
      RetVal));
  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{AHandle}, RetVal));
  EXPECT_GT(std::filesystem::file_size(SandboxDir / "modes.gz"), BaselineSize);

  ASSERT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FreshPtr, ModeWxPtr},
      RetVal));
  const uint32_t XHandle = RetVal[0].get<uint32_t>();
  EXPECT_NE(XHandle, 0U);
  if (XHandle != 0) {
    EXPECT_TRUE(GZClose.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{XHandle},
        RetVal));
  }
  EXPECT_TRUE(std::filesystem::exists(SandboxDir / "fresh.gz"));

  std::filesystem::remove(SandboxDir / "modes.gz", EC);
  std::filesystem::remove(SandboxDir / "fresh.gz", EC);
  std::filesystem::remove(SandboxDir / "transparent.gz", EC);
  std::filesystem::remove(SandboxDir / "gfresh.gz", EC);
}

TEST(WasmEdgeZlibTest, HardeningGZOpenWasiFollowsSandboxSymlinks) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasiWiredModule Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;

  const std::filesystem::path SandboxDir =
      std::filesystem::temp_directory_path() / "wasmedge_zlib_wasi_symlink";
  const std::filesystem::path OutsideFile =
      std::filesystem::temp_directory_path() /
      "wasmedge_zlib_wasi_symlink_outside.gz";
  std::error_code EC;
  std::filesystem::remove_all(SandboxDir, EC);
  std::filesystem::create_directories(SandboxDir, EC);
  std::filesystem::remove(OutsideFile, EC);

  WasmEdge::Host::WasiModule WasiMod;
  WasiMod.init(std::vector<std::string>{SandboxDir.string()}, "test", {}, {});
  Mod.wireWASIModule(&WasiMod);
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  const uint32_t TargetPtr = 16;
  std::strcpy(MemInst.getPointer<char *>(TargetPtr), "target.gz");
  const uint32_t LinkPtr = 32;
  std::strcpy(MemInst.getPointer<char *>(LinkPtr), "link.gz");
  const uint32_t EscLinkPtr = 64;
  std::strcpy(MemInst.getPointer<char *>(EscLinkPtr), "escape.gz");
  const uint32_t AbsLinkPtr = 96;
  std::strcpy(MemInst.getPointer<char *>(AbsLinkPtr), "abslink.gz");
  const uint32_t DangLinkPtr = 128;
  std::strcpy(MemInst.getPointer<char *>(DangLinkPtr), "dangling.gz");
  const uint32_t ModeWPtr = 160;
  std::strcpy(MemInst.getPointer<char *>(ModeWPtr), "wb");
  const uint32_t ModeRPtr = 192;
  std::strcpy(MemInst.getPointer<char *>(ModeRPtr), "rb");
  const uint32_t ModeWxPtr = 224;
  std::strcpy(MemInst.getPointer<char *>(ModeWxPtr), "wx");
  const uint32_t DataPtr = 256;
  const char *const Payload = "symlinked gzip payload";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);
  const uint32_t ReadBuf = 512;

  GET_ZLIB_FUNC(GZOpen, "gzopen")
  GET_ZLIB_FUNC(GZWrite, "gzwrite")
  GET_ZLIB_FUNC(GZRead, "gzread")
  GET_ZLIB_FUNC(GZClose, "gzclose")

  ASSERT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{TargetPtr, ModeWPtr},
      RetVal));
  const uint32_t WHandle = RetVal[0].get<uint32_t>();
  if (WHandle == 0) {
    GTEST_SKIP() << "WASI preopen unavailable in this environment";
  }
  ASSERT_TRUE(GZWrite.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{WHandle, DataPtr, PayloadLen},
      RetVal));
  ASSERT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{WHandle}, RetVal));

  {
    std::ofstream(OutsideFile) << "outside";
  }
  std::filesystem::create_symlink("target.gz", SandboxDir / "link.gz", EC);
  if (EC) {
    GTEST_SKIP() << "cannot create symlinks in this environment";
  }
  std::filesystem::create_symlink(OutsideFile, SandboxDir / "abslink.gz", EC);
  std::filesystem::create_symlink(std::filesystem::path("..") /
                                      OutsideFile.filename(),
                                  SandboxDir / "escape.gz", EC);
  std::filesystem::create_symlink("missing.gz", SandboxDir / "dangling.gz", EC);

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{LinkPtr, ModeRPtr},
      RetVal));
  const uint32_t RHandle = RetVal[0].get<uint32_t>();
  EXPECT_NE(RHandle, 0U);
  if (RHandle != 0) {
    EXPECT_TRUE(GZRead.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               RHandle, ReadBuf, UINT32_C(256)},
                           RetVal));
    EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
    EXPECT_EQ(0, std::memcmp(MemInst.getPointer<char *>(ReadBuf), Payload,
                             PayloadLen));
    EXPECT_TRUE(GZClose.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{RHandle},
        RetVal));
  }

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{LinkPtr, ModeWPtr},
      RetVal));
  const uint32_t LWHandle = RetVal[0].get<uint32_t>();
  EXPECT_NE(LWHandle, 0U);
  if (LWHandle != 0) {
    EXPECT_TRUE(GZClose.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{LWHandle},
        RetVal));
  }
  EXPECT_TRUE(std::filesystem::is_symlink(SandboxDir / "link.gz"));
  EXPECT_TRUE(std::filesystem::exists(SandboxDir / "target.gz"));

  ASSERT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{EscLinkPtr, ModeRPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);
  ASSERT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{AbsLinkPtr, ModeRPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);

  ASSERT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DangLinkPtr, ModeWxPtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);
  EXPECT_FALSE(std::filesystem::exists(SandboxDir / "missing.gz"));

  std::filesystem::remove_all(SandboxDir, EC);
  std::filesystem::remove(OutsideFile, EC);
}

TEST(WasmEdgeZlibTest, HardeningGZDOpenTemporarilyDisabled) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasiWiredModule Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;

  const std::filesystem::path SandboxDir =
      std::filesystem::temp_directory_path() / "wasmedge_zlib_gzdopen_disabled";
  std::error_code EC;
  std::filesystem::create_directories(SandboxDir, EC);
  std::filesystem::remove(SandboxDir / "disabled.gz", EC);

  WasmEdge::Host::WasiModule WasiMod;
  WasiMod.init(std::vector<std::string>{SandboxDir.string()}, "test", {}, {});
  Mod.wireWASIModule(&WasiMod);
  auto *Env = const_cast<WasmEdge::Host::WASI::Environ *>(WasiMod.getEnv());
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  const uint32_t ModeWPtr = 0;
  std::strcpy(MemInst.getPointer<char *>(ModeWPtr), "wb");

  GET_ZLIB_FUNC(GZDOpen, "gzdopen")

  const __wasi_rights_t FullRights =
      __WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_WRITE | __WASI_RIGHTS_FD_SEEK |
      __WASI_RIGHTS_FD_TELL;
  auto Fd =
      Env->pathOpen(3, "disabled.gz", static_cast<__wasi_lookupflags_t>(0),
                    __WASI_OFLAGS_CREAT | __WASI_OFLAGS_TRUNC, FullRights,
                    FullRights, static_cast<__wasi_fdflags_t>(0));
  if (!Fd.has_value()) {
    GTEST_SKIP() << "WASI preopen unavailable in this environment";
  }

  ASSERT_TRUE(GZDOpen.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              static_cast<int32_t>(*Fd), ModeWPtr},
                          RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);

  __wasi_fdstat_t FdStat;
  EXPECT_TRUE(Env->fdFdstatGet(*Fd, FdStat).has_value());
  Env->fdClose(*Fd);

  std::filesystem::remove(SandboxDir / "disabled.gz", EC);
}

TEST(WasmEdgeZlibTest, HardeningGZOpenWasiStreamingWithoutSeekTellRights) {
  auto ZlibMod = createModule();
  ASSERT_TRUE(ZlibMod);

  WasiWiredModule Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1, 1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_NE(MemInstPtr, nullptr);
  auto &MemInst = *MemInstPtr;

  const std::filesystem::path SandboxDir =
      std::filesystem::temp_directory_path() / "wasmedge_zlib_wasi_narrowed";
  std::error_code EC;
  std::filesystem::create_directories(SandboxDir, EC);
  std::filesystem::remove(SandboxDir / "stream.gz", EC);
  std::filesystem::remove(SandboxDir / "fresh.gz", EC);
  std::filesystem::remove(SandboxDir / "append.gz", EC);

  WasmEdge::Host::WasiModule WasiMod;
  WasiMod.init(std::vector<std::string>{SandboxDir.string()}, "test", {}, {});
  Mod.wireWASIModule(&WasiMod);
  auto *Env = const_cast<WasmEdge::Host::WASI::Environ *>(WasiMod.getEnv());
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  const uint32_t PathPtr = 16;
  std::strcpy(MemInst.getPointer<char *>(PathPtr), "stream.gz");
  const uint32_t FreshPtr = 32;
  std::strcpy(MemInst.getPointer<char *>(FreshPtr), "fresh.gz");
  const uint32_t AppendPtr = 64;
  std::strcpy(MemInst.getPointer<char *>(AppendPtr), "append.gz");
  const uint32_t ModeWPtr = 96;
  std::strcpy(MemInst.getPointer<char *>(ModeWPtr), "wb");
  const uint32_t ModeRPtr = 112;
  std::strcpy(MemInst.getPointer<char *>(ModeRPtr), "rb");
  const uint32_t ModeAPtr = 128;
  std::strcpy(MemInst.getPointer<char *>(ModeAPtr), "ab");
  const uint32_t DataPtr = 256;
  const char *const Payload = "streaming without seek rights";
  const uint32_t PayloadLen = static_cast<uint32_t>(std::strlen(Payload));
  std::strcpy(MemInst.getPointer<char *>(DataPtr), Payload);
  const uint32_t ReadPtr = 512;

  GET_ZLIB_FUNC(GZOpen, "gzopen")
  GET_ZLIB_FUNC(GZWrite, "gzwrite")
  GET_ZLIB_FUNC(GZRead, "gzread")
  GET_ZLIB_FUNC(GZClose, "gzclose")
  GET_ZLIB_FUNC(GZSeek, "gzseek")
  GET_ZLIB_FUNC(GZOffset, "gzoffset")
  GET_ZLIB_FUNC(GZTell, "gztell")

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeWPtr},
      RetVal));
  const uint32_t WHandle = RetVal[0].get<uint32_t>();
  if (WHandle == 0) {
    GTEST_SKIP() << "WASI-mediated gzopen unavailable in this environment";
  }
  ASSERT_TRUE(GZWrite.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{WHandle, DataPtr, PayloadLen},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
  ASSERT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{WHandle}, RetVal));

  __wasi_fdstat_t DirStat;
  ASSERT_TRUE(Env->fdFdstatGet(3, DirStat).has_value());
  const __wasi_rights_t NoSeekTell =
      ~(__WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_TELL);
  ASSERT_TRUE(Env->fdFdstatSetRights(3, DirStat.fs_rights_base & NoSeekTell,
                                     DirStat.fs_rights_inheriting & NoSeekTell)
                  .has_value());

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeRPtr},
      RetVal));
  const uint32_t RHandle = RetVal[0].get<uint32_t>();
  ASSERT_NE(RHandle, 0U);
  ASSERT_TRUE(GZRead.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{RHandle, ReadPtr, PayloadLen},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
  EXPECT_EQ(
      0, std::memcmp(MemInst.getPointer<char *>(ReadPtr), Payload, PayloadLen));
  ASSERT_TRUE(GZSeek.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{
                             RHandle, INT32_C(6), INT32_C(SEEK_SET)},
                         RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), -1);
  ASSERT_TRUE(GZOffset.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{RHandle}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), -1);
  ASSERT_TRUE(GZTell.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{RHandle}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
  ASSERT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{RHandle}, RetVal));

  ASSERT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FreshPtr, ModeWPtr}, RetVal));
  const uint32_t FHandle = RetVal[0].get<uint32_t>();
  ASSERT_NE(FHandle, 0U);
  ASSERT_TRUE(GZWrite.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FHandle, DataPtr, PayloadLen},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
  ASSERT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FHandle}, RetVal));
  EXPECT_TRUE(std::filesystem::exists(SandboxDir / "fresh.gz"));

  ASSERT_TRUE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{AppendPtr, ModeAPtr},
      RetVal));
  const uint32_t AHandle = RetVal[0].get<uint32_t>();
  ASSERT_NE(AHandle, 0U);
  ASSERT_TRUE(GZWrite.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{AHandle, DataPtr, PayloadLen},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(PayloadLen));
  ASSERT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{AHandle}, RetVal));

  std::filesystem::remove(SandboxDir / "stream.gz", EC);
  std::filesystem::remove(SandboxDir / "fresh.gz", EC);
  std::filesystem::remove(SandboxDir / "append.gz", EC);
}

GTEST_API_ int main(int ArgC, char **ArgV) {
  testing::InitGoogleTest(&ArgC, ArgV);
  return RUN_ALL_TESTS();
}
