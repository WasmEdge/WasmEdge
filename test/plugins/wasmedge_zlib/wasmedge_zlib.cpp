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
