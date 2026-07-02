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
