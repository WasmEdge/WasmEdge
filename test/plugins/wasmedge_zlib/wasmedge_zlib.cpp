// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/defines.h"
#include "runtime/instance/module.h"
#include "zlibfunc.h"
#include "zlibmodule.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
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

  // Out-of-bounds z_stream pointer must fail instead of dereferencing NULL:
  // 65530 + sizeof(WasmZStream) exceeds the single memory page.
  {
    GET_ZLIB_FUNC(DeflateInit, "deflateInit")
    EXPECT_FALSE(DeflateInit.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     UINT32_C(65530), INT32_C(-1)},
                                 RetVal));
  }

  // A guest-controlled avail_out larger than linear memory must be rejected
  // rather than letting zlib write past the end of memory.
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

  // compress with an out-of-bounds destination-length pointer must fail.
  {
    GET_ZLIB_FUNC(Compress, "compress")
    EXPECT_FALSE(Compress.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(65534), UINT32_C(100), UINT32_C(10)},
        RetVal));
  }

  // compress with a valid destination but an out-of-bounds source length must
  // fail rather than let zlib read past the end of memory.
  {
    GET_ZLIB_FUNC(Compress, "compress")
    *MemInst.getPointer<uint32_t *>(0) = 100;
    EXPECT_FALSE(Compress.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(8), UINT32_C(0), UINT32_C(4000), UINT32_C(0xFFFFFFF0)},
        RetVal));
  }

  // uncompress with an out-of-bounds destination-length pointer must fail.
  {
    GET_ZLIB_FUNC(Uncompress, "uncompress")
    EXPECT_FALSE(Uncompress.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            UINT32_C(0), UINT32_C(65535), UINT32_C(100), UINT32_C(10)},
        RetVal));
  }

  // adler32 / crc32 with a length larger than memory must fail rather than
  // read out of bounds.
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

  // An unknown gz file handle must be rejected, not dereferenced.
  {
    GET_ZLIB_FUNC(GZRead, "gzread")
    EXPECT_FALSE(GZRead.run(CallFrame,
                            std::initializer_list<WasmEdge::ValVariant>{
                                UINT32_C(9999), UINT32_C(0), UINT32_C(10)},
                            RetVal));
  }
}

TEST(WasmEdgeZlibTest, HardeningCompress2AnswersInvalidLevelBeforeBounds) {
  // An out-of-range level fails compress2's deflateInit before any source
  // byte is read or dest byte written -- though only after *destLen has been
  // zeroed -- so the wrapper must surface that Z_STREAM_ERROR instead of
  // trapping on data spans the rejected call would never touch. A level zlib
  // accepts keeps the bounds check.
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

  // A garbage *destLen makes the dest span run far past linear memory.
  const uint32_t DestLenPtr = 0;
  *MemInst.getPointer<uint32_t *>(DestLenPtr) = UINT32_C(0xFFFFFFF0);
  ASSERT_TRUE(Compress2.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(8), DestLenPtr, UINT32_C(4096), UINT32_C(64), INT32_C(10)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);
  EXPECT_EQ(*MemInst.getPointer<uint32_t *>(DestLenPtr), UINT32_C(0));

  // With an accepted level the same out-of-bounds span must keep trapping.
  *MemInst.getPointer<uint32_t *>(DestLenPtr) = UINT32_C(0xFFFFFFF0);
  EXPECT_FALSE(Compress2.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          UINT32_C(8), DestLenPtr, UINT32_C(4096), UINT32_C(64), INT32_C(-1)},
      RetVal));
}

TEST(WasmEdgeZlibTest, HardeningUncompress2AliasedLengthMatchesNative) {
  // Native uncompress2 writes *destLen last, so when a guest aliases the two
  // length pointers (DestLenPtr == SourceLenPtr) the shared slot ends holding
  // the produced (uncompressed) length, not the consumed source length. The
  // wrapper snapshots into separate host variables, so it must write them back
  // in the same order to leave the guest the value native would.
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

  // A highly compressible payload, compressed to a zlib stream.
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

  // Aliased call: the single slot is both dest capacity and source length. A
  // capacity that covers the produced and compressed sizes lets the decode
  // finish; the wrapper bounds the source by this length, and inflate stops at
  // the stream end long before consuming the zero-filled tail.
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

  // Non-aliased control: each slot keeps its own result.
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
  // compress distinguishes a null destination (Z_STREAM_ERROR) from a
  // non-null one with no capacity (Z_BUF_ERROR) without writing a byte, so
  // the wrapper must not turn either answer into the other: a guest null maps
  // to Z_NULL while a nonzero offset stays non-null even when its zero-length
  // span carries no pointer (one out of bounds included).
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

  // A guest-null destination with no capacity is zlib's Z_STREAM_ERROR.
  *MemInst.getPointer<uint32_t *>(DestLenPtr) = 0;
  ASSERT_TRUE(Compress.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               UINT32_C(0), DestLenPtr, SourcePtr, PayloadLen},
                           RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);

  // A nonzero destination offset past the end of linear memory still answers
  // Z_BUF_ERROR at zero capacity; the pointer is never dereferenced.
  *MemInst.getPointer<uint32_t *>(DestLenPtr) = 0;
  ASSERT_TRUE(
      Compress.run(CallFrame,
                   std::initializer_list<WasmEdge::ValVariant>{
                       UINT32_C(0xFFFF0000), DestLenPtr, SourcePtr, PayloadLen},
                   RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_BUF_ERROR);

  // An in-bounds destination with no capacity keeps the same answer.
  *MemInst.getPointer<uint32_t *>(DestLenPtr) = 0;
  ASSERT_TRUE(Compress.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               UINT32_C(8), DestLenPtr, SourcePtr, PayloadLen},
                           RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_BUF_ERROR);
}

TEST(WasmEdgeZlibTest, HardeningZeroLengthDictionaryMatchesZlib) {
  // deflateSetDictionary never reads a zero-length dictionary yet still
  // rejects Z_NULL, so the wrapper must not turn either case into the other:
  // any nonzero guest offset stays acceptable at length zero, while a guest
  // null keeps failing with Z_STREAM_ERROR the way it does natively.
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

  // Beyond the single memory page, so the zero-length span has no pointer.
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

  // A nonzero length keeps the bounds check: the same wild offset must fail.
  EXPECT_FALSE(
      DeflateSetDictionary.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ZS, UINT32_C(0xFFFF0000), UINT32_C(64)},
                               RetVal));

  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
}

TEST(WasmEdgeZlibTest, HardeningZeroLengthChecksumMatchesZlib) {
  // adler32/crc32 read Z_NULL as "return the initial value" and any non-null
  // zero-length buffer as "return the running value unchanged". A guest null
  // (offset 0) must map to Z_NULL, while any other zero-length offset -- even a
  // wild one past the end of memory whose span carries no pointer -- must
  // preserve the caller's checksum instead of resetting it to the seed.
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

  // Guest Z_NULL (offset 0, length 0) returns the checksum's initial value.
  // zlib defines adler32(0, Z_NULL, 0) == 1 and crc32(0, Z_NULL, 0) == 0.
  ASSERT_TRUE(Adler32.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              Running, UINT32_C(0), UINT32_C(0)},
                          RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), INT32_C(1));

  // A non-null zero-length buffer, including a wild offset past memory whose
  // span has no pointer, preserves the running value rather than seeding.
  ASSERT_TRUE(Adler32.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              Running, WildOffset, UINT32_C(0)},
                          RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), static_cast<int32_t>(Running));

  // The same wild offset with a nonzero length still fails the bounds check.
  EXPECT_FALSE(Adler32.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               Running, WildOffset, UINT32_C(1)},
                           RetVal));

  // crc32 shares the contract; its initial value is 0 rather than 1.
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

  // Fill every byte of linear memory with a non-NUL value so any C string
  // starting inside it has no terminator before the end of memory.
  std::fill_n(MemInst.getPointer<uint8_t *>(0),
              static_cast<size_t>(MemInst.getSize()),
              static_cast<uint8_t>(0xFF));

  GET_ZLIB_FUNC(GZOpen, "gzopen")
  EXPECT_FALSE(GZOpen.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(100)},
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
  ASSERT_LT(TmpPath.size() + 1, 1024U);

  const uint32_t PathPtr = 0;
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

  // Closing must free the zlib handle exactly once (no double-free).
  EXPECT_TRUE(GZClose.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{WHandle}, RetVal));

  ASSERT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModeRPtr},
      RetVal));
  const uint32_t RHandle = RetVal[0].get<uint32_t>();
  EXPECT_NE(RHandle, WHandle);

  // gzread with an out-of-bounds length must fail rather than overflow the
  // buffer.
  EXPECT_FALSE(GZRead.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              RHandle, UINT32_C(60000), UINT32_C(0xFFFFFFFF)},
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

  // A double close must be rejected gracefully; the handle is already gone.
  EXPECT_FALSE(GZClose.run(
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

  // After the header is captured, corrupt the guest name and repoint the header
  // field to an unterminated region at the end of memory. With the snapshot the
  // emitted name must stay "original-header-name"; without it, deflate would
  // re-read this and run off the end of linear memory.
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

  // The round-tripped header name is the snapshot taken at set-header time, not
  // the corrupted guest value.
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

  // A gzopen that fails for an ordinary I/O reason (a missing file) must return
  // a null handle (0) so guest code can handle the usual gzopen(...) == NULL
  // case, rather than trapping the whole Wasm call.
  const std::string MissingPath =
      (std::filesystem::temp_directory_path() / "wasmedge_zlib_absent_dir_zzz" /
       "definitely_absent.gz")
          .string();
  ASSERT_LT(MissingPath.size() + 1, 900U);
  const uint32_t PathPtr = 0;
  std::strcpy(MemInst.getPointer<char *>(PathPtr), MissingPath.c_str());
  const uint32_t ModePtr = 1000;
  std::strcpy(MemInst.getPointer<char *>(ModePtr), "rb");

  GET_ZLIB_FUNC(GZOpen, "gzopen")
  EXPECT_TRUE(GZOpen.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PathPtr, ModePtr},
      RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);

  // gzdopen on an invalid file descriptor is likewise an ordinary failure.
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

  // The first set-header succeeds and is captured into host storage; zlib now
  // holds a pointer into that stored header.
  auto *DHeader = MemInst.getPointer<WasmGZHeader *>(DHdr);
  DHeader->Name = Name1Ptr;
  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, DHdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  // The second set-header names a header whose name runs off the end of linear
  // memory, so the snapshot is rejected. The failed replacement must not
  // clobber the previously stored, still-referenced header.
  DHeader->Name = static_cast<uint32_t>(MemInst.getSize()) - 3;
  std::fill_n(MemInst.getPointer<uint8_t *>(
                  static_cast<uint32_t>(MemInst.getSize()) - 3),
              3, static_cast<uint8_t>(0xFF));
  EXPECT_FALSE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS, DHdr},
      RetVal));

  // The stored header is still the first one, not the failed replacement.
  const auto &HeaderMap = ZlibMod->getEnv().GZHeaderMap;
  const auto StoreIt = HeaderMap.find(DZS);
  ASSERT_NE(StoreIt, HeaderMap.end());
  EXPECT_EQ(StoreIt->second->Name, Name1);
}

TEST(WasmEdgeZlibTest, HardeningDeflateSetHeaderNonGzipSkipsSnapshot) {
  // zlib only accepts deflateSetHeader on a gzip deflate stream and answers
  // Z_STREAM_ERROR for anything else without reading the header. The host must
  // obtain that verdict before doing any guest-header work: on a zlib-format
  // stream, even a header whose name would fail the snapshot's bounds check
  // (or force a guest-sized host copy) surfaces zlib's Z_STREAM_ERROR instead
  // of trapping or copying.
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

  // windowBits 15 requests the zlib wrapper, not gzip, so zlib will refuse
  // the header on this stream.
  ASSERT_TRUE(
      DeflateInit2.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{
                           DZS, INT32_C(-1), INT32_C(Z_DEFLATED), INT32_C(15),
                           INT32_C(8), INT32_C(Z_DEFAULT_STRATEGY)},
                       RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  // The header name runs unterminated to the end of linear memory; on a gzip
  // stream this snapshot would trap. zlib's verdict comes first, so the call
  // reports Z_STREAM_ERROR and stores nothing.
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
  // deflateEnd / inflateEnd must drop the per-stream gzip-header snapshot.
  // Leaving it in GZHeaderMap lets a guest accumulate host allocations by
  // cycling stream pointers, and would keep a copied stream's shared header
  // alive past the point zlib still needs it.
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
  // Every inflate reset detaches zlib's internal gzip-header pointer (the
  // guest must call inflateGetHeader again for the next stream), so the host
  // snapshot must be dropped with it. A stale entry keeps re-validating and
  // rewriting the guest header on every inflate call, trapping legitimate
  // post-reset use once the guest reuses that memory.
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

  // Build a small gzip stream to decode after the resets.
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

  // After a reset the guest may legitimately reuse the header memory; inflate
  // must not trap on the discarded registration.
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
  // deflateCopy duplicates zlib's internal gz_header pointer into the copied
  // stream. Replacing (or ending) the source header must not free the storage
  // the copy still references, otherwise a later deflate on the copy reads
  // freed host memory.
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

  // Copy the freshly configured source stream.
  ASSERT_TRUE(DeflateCopy.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DstZS, SrcZS},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  const auto &HeaderMap = ZlibMod->getEnv().GZHeaderMap;
  // The copy must own an independent, live header snapshot.
  {
    const auto DstIt = HeaderMap.find(DstZS);
    ASSERT_NE(DstIt, HeaderMap.end());
    EXPECT_EQ(DstIt->second->Name, OrigName);
  }

  // Replace the source header; on the buggy code this frees the storage the
  // copied stream still points at.
  DHeader->Name = Name2Ptr;
  ASSERT_TRUE(DeflateSetHeader.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{SrcZS, Hdr},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  // The copy still carries its own original header, independent of the source.
  {
    const auto DstIt = HeaderMap.find(DstZS);
    ASSERT_NE(DstIt, HeaderMap.end());
    EXPECT_EQ(DstIt->second->Name, OrigName);
  }

  // Drive the copied stream to completion; it must emit the original name
  // without reading freed memory.
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

  // Round-trip the copy's output and confirm the header name survived.
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
  // inflateCopy duplicates zlib's internal gz_header pointer into the copied
  // stream, exactly like deflateCopy. Ending the source stream must not free
  // the header snapshot the copy still points at, otherwise a later inflate on
  // the copy writes the parsed gzip header through freed host memory.
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

  // The copy must share the source's live header snapshot; its internal head
  // pointer now aliases the same host storage.
  const auto &HeaderMap = ZlibMod->getEnv().GZHeaderMap;
  const auto SrcIt = HeaderMap.find(SrcZS);
  const auto DstIt = HeaderMap.find(DstZS);
  ASSERT_NE(SrcIt, HeaderMap.end());
  ASSERT_NE(DstIt, HeaderMap.end());
  EXPECT_EQ(DstIt->second, SrcIt->second);

  // Ending the source must keep the shared snapshot alive for the copy.
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{SrcZS}, RetVal));
  EXPECT_EQ(HeaderMap.find(SrcZS), HeaderMap.end());
  EXPECT_NE(HeaderMap.find(DstZS), HeaderMap.end());

  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DstZS}, RetVal));
}

TEST(WasmEdgeZlibTest, InflateGetHeaderReplacesStoredHeaderOnSuccess) {
  // A second inflateGetHeader on the same stream re-points zlib's internal head
  // at the new host snapshot. The stored snapshot must be replaced (and kept
  // alive), otherwise a later inflate writes the header through freed memory.
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

  // Re-register a different guest header. zlib now points at the second
  // snapshot, so the stored entry must become the second one.
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
  // deflateEnd frees all of zlib's internal state even when it returns
  // Z_DATA_ERROR for a stream ended mid-compression. The host tracking (stream
  // entry and gzip-header snapshot) must be dropped on that path too, otherwise
  // a guest can accumulate host allocations by cycling stream pointers.
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

  // Compress part of the input without finishing; the stream is now mid-flight.
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

  // deflateEnd on a mid-compression stream returns Z_DATA_ERROR but still frees
  // zlib's state; the host entries must be erased on that path too.
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DZS}, RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_DATA_ERROR);
  EXPECT_EQ(Env.ZStreamMap.find(DZS), Env.ZStreamMap.end());
  EXPECT_EQ(Env.GZHeaderMap.find(DZS), Env.GZHeaderMap.end());
}

TEST(WasmEdgeZlibTest, InflateSyncAllowsDirtyOutputBuffer) {
  // inflateSync consumes input but never writes output ("No output is
  // provided"), so a stale/out-of-bounds next_out must not trap the call. The
  // input buffer is still validated because inflateSync scans next_in.
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

  // Valid input, but next_out points one past the end of linear memory with a
  // nonzero avail_out. inflateSync must still run rather than trap.
  auto *IStrm = MemInst.getPointer<WasmZStream *>(IZS);
  IStrm->NextIn = InPtr;
  IStrm->AvailIn = 8;
  IStrm->NextOut = static_cast<uint32_t>(MemInst.getSize());
  IStrm->AvailOut = 16;
  EXPECT_TRUE(InflateSync.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_DATA_ERROR);

  // The input buffer stays guarded: an out-of-bounds next_in with nonzero
  // avail_in must still trap.
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
  // zlib's contract lets a guest call deflateInit / reset / end with
  // uninitialized next_in/avail_in (only zalloc/zfree/opaque must be set).
  // Those control operations do not consume the data buffers, so an
  // out-of-bounds next_in/next_out must not trap them; only the streaming
  // operations validate the buffers.
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
    // next_in + avail_in runs off the end of linear memory.
    Strm->NextIn = MemBytes - 4;
    Strm->AvailIn = 0x10000;
    Strm->NextOut = MemBytes - 4;
    Strm->AvailOut = 0x10000;
  };

  // Initialize the stream with dirty, out-of-bounds buffer fields present.
  SetDirtyBuffers();
  ASSERT_TRUE(DeflateInit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-1)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  // reset ignores the data buffers too.
  SetDirtyBuffers();
  ASSERT_TRUE(DeflateReset.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);

  // The streaming path must still reject an out-of-bounds output buffer.
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

  // End cleans up even with dirty buffer fields.
  SetDirtyBuffers();
  ASSERT_TRUE(DeflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
}

TEST(WasmEdgeZlibTest, HardeningRejectedParamsIgnoreDirtyDataBuffers) {
  // deflateParams rejects an out-of-range level or strategy -- and deflate()
  // an out-of-range flush -- before consuming next_in or producing into
  // next_out, so a call zlib refuses must surface its Z_STREAM_ERROR instead
  // of trapping on dirty buffer fields; serviceable parameters keep the
  // bounds check.
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

  // An out-of-range level is zlib's Z_STREAM_ERROR, not a trap.
  SetDirtyBuffers();
  ASSERT_TRUE(DeflateParams.run(CallFrame,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    ZS, INT32_C(10), INT32_C(Z_FILTERED)},
                                RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);

  // An out-of-range strategy behaves the same.
  SetDirtyBuffers();
  ASSERT_TRUE(DeflateParams.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(-1), INT32_C(99)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);

  // deflate() with a flush beyond Z_BLOCK is rejected the same way.
  SetDirtyBuffers();
  ASSERT_TRUE(Deflate.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{ZS, INT32_C(99)},
      RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_ERROR);

  // Parameters zlib would service must still reject the dirty buffers.
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

  // An out-of-bounds HeadPtr must be rejected up front (like deflateSetHeader)
  // rather than stored and dereferenced on later inflate/cleanup calls:
  // 65530 + sizeof(WasmGZHeader) exceeds the single 64 KiB page.
  EXPECT_FALSE(InflateGetHeader.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{IZS, UINT32_C(65530)},
      RetVal));
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

  // The guest corrupts NameMax to an out-of-bounds span after registering the
  // header. inflateEnd is a cleanup call that never writes the gzip header, so
  // it must still run zlib's inflateEnd and release the stream rather than
  // aborting on header-buffer validation (which would leak the internal state).
  IHeader->NameMax = UINT32_C(0xFFFFFFFF);
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), Z_OK);
}

TEST(WasmEdgeZlibTest, GZHeaderAbsentOptionalFieldsSurfaceAsNull) {
  // zlib clears gz_header.extra/name/comment to Z_NULL when the stream lacks
  // the field even though the caller supplied buffers. The write-back must
  // surface that as guest null (0) instead of subtracting a live host pointer
  // from null, which poisoned the guest header and made every later inflate
  // call on the stream trap.
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

  // The default gzip header carries no extra, name, or comment field.
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

  // The first 12 bytes cover the whole field-free header, so zlib completes it
  // and clears the absent optional fields within this call.
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

  // The stream must stay usable after the header write-back.
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
  // A guest null (0) extra/name/comment pointer means the field is not
  // requested (zlib's Z_NULL contract) and must not resolve to wasm address 0:
  // with a nonzero *Max, zlib would copy header contents into guest memory
  // the guest never offered.
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

  // Nothing may have been written through the null field pointers.
  const std::vector<uint8_t> Expected(64, 0xAB);
  EXPECT_EQ(0,
            std::memcmp(MemInst.getPointer<uint8_t *>(0), Expected.data(), 64));

  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
}

TEST(WasmEdgeZlibTest, DeflateSetHeaderNullRevertsToDefaultHeader) {
  // Native deflateSetHeader accepts Z_NULL and reverts the stream to the
  // default gzip header; guest 0 must take that path instead of snapshotting
  // whatever bytes sit at wasm address 0 (and trapping on garbage there).
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
  // Poison wasm address 0: a wrapper that dereferences the null header would
  // read a header full of out-of-bounds pointers and trap.
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

  // Build a gzip stream carrying a 100-byte extra field with distinctive,
  // non-zero content so a misplaced copy is detectable.
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

  // Inflate it in two chunks, injecting a bogus ExtraLen between them.
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

  // First chunk: exactly the 10-byte gzip base header + 2-byte XLEN, so zlib
  // parses EXLEN (setting extra_len = 100 from the stream) and parks in the
  // EXTRA state before copying any extra bytes.
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

  // extra_len is an OUTPUT field owned by zlib (true XLEN is 100). A malicious
  // guest overwrites it before the continuation call. The plugin must not push
  // this back into zlib's header, or the guest controls the EXTRA-state write
  // offset (len = extra_len - state->length) -- CVE-2022-37434 territory on
  // zlib <= 1.2.12.
  IHeader->ExtraLen = 150;

  IStrm->AvailIn = CompLen - 12;
  ASSERT_TRUE(Inflate.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{IZS, INT32_C(Z_FINISH)},
      RetVal));
  ASSERT_EQ(RetVal[0].get<int32_t>(), Z_STREAM_END);
  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));

  // zlib's parsed extra_len (100) must survive rather than the injected 150,
  // and the extra bytes must land at offset 0 (len == 0), not offset 50.
  EXPECT_EQ(IHeader->ExtraLen, ExtraFieldLen);
  EXPECT_EQ(*MemInst.getPointer<uint8_t *>(ExtraDst), static_cast<uint8_t>(1));
}

TEST(WasmEdgeZlibTest, HardeningInflateHeaderZeroCapacityKeepsGuestPointers) {
  // zlib never writes through a zero-capacity extra/name/comment buffer, yet
  // still distinguishes the caller's non-null pointer from Z_NULL ("field not
  // requested"): the pointer survives untouched and only an absent stream
  // field nulls it. A stale zero-capacity guest pointer past linear memory is
  // therefore legal input -- the header sync must not misread it as Z_NULL
  // and write guest NULL back over it.
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

  // Build a gzip stream that carries all three optional header fields.
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

  // Inflate with non-null zero-capacity pointers that lie outside linear
  // memory: legal for zlib (never dereferenced at *_max == 0), so they must
  // come back untouched, with the output fields still populated.
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

  // The guest's pointers survive; the stream's fields still surface through
  // the zlib-owned outputs.
  EXPECT_EQ(IHeader->Extra, OOBExtra);
  EXPECT_EQ(IHeader->Name, OOBName);
  EXPECT_EQ(IHeader->Comment, OOBComment);
  EXPECT_EQ(IHeader->ExtraLen, ExtraFieldLen);
  EXPECT_EQ(IHeader->Done, INT32_C(1));

  ASSERT_TRUE(InflateEnd.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{IZS}, RetVal));
}


GTEST_API_ int main(int ArgC, char **ArgV) {
  testing::InitGoogleTest(&ArgC, ArgV);
  return RUN_ALL_TESTS();
}
