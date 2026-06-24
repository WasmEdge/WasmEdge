// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avutil/avDictionary.h"
#include "avutil/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVDictionary) {
  using namespace std::literals::string_view_literals;

  uint32_t KeyStart = UINT32_C(1);
  uint32_t KeyLen = 3;
  uint32_t ValueStart = UINT32_C(4);
  uint32_t ValueLen = 5;
  uint32_t PrevDictEntryIdx = 0; // The Fetch the next Key value Node using an
  // index. Passing Index from Rust side.
  int32_t Flags = 0;
  uint32_t NullDictId = UINT32_C(0);

  uint32_t DictPtr = UINT32_C(80);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_set");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDictSet = FuncInst->getHostFunc();

  // Fill 0 in WasmMemory.
  fillMemContent(MemInst, KeyStart, KeyLen + ValueLen);
  fillMemContent(MemInst, KeyStart, "KEY"sv);
  fillMemContent(MemInst, ValueStart, "VALUE"sv);

  // Storing the above Key and Value in dict and using these in below tests
  // (dict_get) to fetch Key,values.
  {
    EXPECT_TRUE(HostFuncAVDictSet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            DictPtr, KeyStart, KeyLen, ValueStart, ValueLen, Flags},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() >= 0);
    ASSERT_TRUE(readUInt32(MemInst, DictPtr) > 0);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_copy");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDictCopy = FuncInst->getHostFunc();

  {
    uint32_t DestDictPtr = UINT32_C(80);
    uint32_t SrcDictId = readUInt32(MemInst, DictPtr);
    EXPECT_TRUE(
        HostFuncAVDictCopy.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   DestDictPtr, SrcDictId, Flags},
                               Result));
    ASSERT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_get");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDictGet = FuncInst->getHostFunc();

  {
    // Store the string lengths of Key and value in the pointers below.
    uint32_t KeyLenPtr = UINT32_C(56);
    uint32_t ValueLenPtr = UINT32_C(60);
    uint32_t DictId = readUInt32(MemInst, DictPtr);
    EXPECT_TRUE(HostFuncAVDictGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{DictId, KeyStart, KeyLen,
                                                    PrevDictEntryIdx, Flags,
                                                    KeyLenPtr, ValueLenPtr},
        Result));
    EXPECT_TRUE(Result[0].get<int32_t>() == 1);
    EXPECT_EQ(readUInt32(MemInst, KeyLenPtr), KeyLen);
    EXPECT_EQ(readUInt32(MemInst, ValueLenPtr), ValueLen);

    // Pass a Null Dict and testing.
    EXPECT_TRUE(HostFuncAVDictGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            NullDictId, KeyStart, KeyLen, PrevDictEntryIdx, Flags, KeyLenPtr,
            ValueLenPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(),
              static_cast<int32_t>(ErrNo::InternalError));
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_dict_get_key_value");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDictGetKeyValue = FuncInst->getHostFunc();

  {
    // Store the strings of Key and value in the buffer pointers below.
    uint32_t KeyBufPtr = UINT32_C(36);
    uint32_t ValueBufPtr = UINT32_C(40);
    uint32_t DictId = readUInt32(MemInst, DictPtr);
    EXPECT_TRUE(HostFuncAVDictGetKeyValue.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            DictId, KeyStart, KeyLen, ValueBufPtr, ValueLen, KeyBufPtr,
            UINT32_C(3), PrevDictEntryIdx, Flags},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(KeyBufPtr),
                               static_cast<size_t>(KeyLen)),
              "KEY"sv);
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(ValueBufPtr),
                               static_cast<size_t>(ValueLen)),
              "VALUE"sv);

    // Pass a Null Dict and testing.
    EXPECT_TRUE(HostFuncAVDictGetKeyValue.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            NullDictId, KeyStart, KeyLen, ValueBufPtr, ValueLen, KeyBufPtr,
            UINT32_C(3), PrevDictEntryIdx, Flags},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(),
              static_cast<int32_t>(ErrNo::InternalError));
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_free");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDictFree = FuncInst->getHostFunc();

  {
    uint32_t DictId = readUInt32(MemInst, DictPtr);
    EXPECT_TRUE(HostFuncAVDictFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{DictId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

TEST_F(FFmpegTest, AVDictGetKeyValueBounds) {
  using namespace std::literals::string_view_literals;
  ASSERT_TRUE(AVUtilMod != nullptr);

  uint32_t KeyStart = UINT32_C(1);
  uint32_t KeyLen = 3;
  uint32_t ValueStart = UINT32_C(4);
  uint32_t ValueLen = 5;
  int32_t Flags = 0;
  uint32_t DictPtr = UINT32_C(80);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_set");
  auto &HostFuncAVDictSet =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictSet &>(
          FuncInst->getHostFunc());

  fillMemContent(MemInst, KeyStart, KeyLen + ValueLen);
  fillMemContent(MemInst, KeyStart, "KEY"sv);
  fillMemContent(MemInst, ValueStart, "VALUE"sv);
  HostFuncAVDictSet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DictPtr, KeyStart, KeyLen,
                                                  ValueStart, ValueLen, Flags},
      Result);
  ASSERT_TRUE(readUInt32(MemInst, DictPtr) > 0);

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_dict_get_key_value");
  auto &HostFuncAVDictGetKeyValue =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictGetKeyValue &>(
          FuncInst->getHostFunc());

  // Destination buffers are deliberately smaller than the stored key/value and
  // fenced with a sentinel; the host must clamp the copy to the given lengths.
  uint32_t KeyBufPtr = UINT32_C(200);
  uint32_t ValBufPtr = UINT32_C(220);
  uint32_t KeyBufLen = UINT32_C(1); // < strlen("KEY")
  uint32_t ValBufLen = UINT32_C(2); // < strlen("VALUE")
  uint32_t FenceLen = UINT32_C(16);
  fillMemContent(MemInst, KeyBufPtr, FenceLen, UINT8_C(0xAA));
  fillMemContent(MemInst, ValBufPtr, FenceLen, UINT8_C(0xAA));

  uint32_t DictId = readUInt32(MemInst, DictPtr);
  HostFuncAVDictGetKeyValue.run(CallFrame,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    DictId, KeyStart, KeyLen, ValBufPtr,
                                    ValBufLen, KeyBufPtr, KeyBufLen,
                                    UINT32_C(0), Flags},
                                Result);
  EXPECT_EQ(Result[0].get<int32_t>(), 1);

  char *ValBuf = MemInst->getPointer<char *>(ValBufPtr);
  for (uint32_t I = ValBufLen; I < FenceLen; ++I) {
    EXPECT_EQ(static_cast<uint8_t>(ValBuf[I]), UINT8_C(0xAA));
  }
  char *KeyBuf = MemInst->getPointer<char *>(KeyBufPtr);
  for (uint32_t I = KeyBufLen; I < FenceLen; ++I) {
    EXPECT_EQ(static_cast<uint8_t>(KeyBuf[I]), UINT8_C(0xAA));
  }
}

TEST_F(FFmpegTest, AVDictGetKeyValueKeyLenBounds) {
  using namespace std::literals::string_view_literals;
  ASSERT_TRUE(AVUtilMod != nullptr);

  uint32_t KeyStart = UINT32_C(1);
  uint32_t KeyLen = 3;
  uint32_t ValueStart = UINT32_C(4);
  uint32_t ValueLen = 5;
  int32_t Flags = 0;
  uint32_t DictPtr = UINT32_C(80);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_set");
  auto &HostFuncAVDictSet =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictSet &>(
          FuncInst->getHostFunc());

  fillMemContent(MemInst, KeyStart, "KEY"sv);
  fillMemContent(MemInst, ValueStart, "VALUE"sv);
  HostFuncAVDictSet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DictPtr, KeyStart, KeyLen,
                                                  ValueStart, ValueLen, Flags},
      Result);
  ASSERT_TRUE(readUInt32(MemInst, DictPtr) > 0);
  uint32_t DictId = readUInt32(MemInst, DictPtr);

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_dict_get_key_value");
  auto &HostFuncAVDictGetKeyValue =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictGetKeyValue &>(
          FuncInst->getHostFunc());

  // The lookup-key pointer is in bounds but the guest-declared key length runs
  // off the end of linear memory. The host must reject the call instead of
  // reading KeyLen bytes from a pointer validated for only one byte.
  uint32_t KeyBufPtr = UINT32_C(200);
  uint32_t ValBufPtr = UINT32_C(220);
  uint32_t KeyBufLen = UINT32_C(8);
  uint32_t ValBufLen = UINT32_C(8);
  uint32_t OutOfBoundsKeyPtr = UINT32_C(65000);
  uint32_t OutOfBoundsKeyLen = UINT32_C(2000);
  HostFuncAVDictGetKeyValue.run(CallFrame,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    DictId, OutOfBoundsKeyPtr,
                                    OutOfBoundsKeyLen, ValBufPtr, ValBufLen,
                                    KeyBufPtr, KeyBufLen, UINT32_C(0), Flags},
                                Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::MissingMemory));
}

TEST_F(FFmpegTest, AVDictGetKeyLenBounds) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  int32_t Flags = 0;
  uint32_t KeyLenPtr = UINT32_C(200);
  uint32_t ValueLenPtr = UINT32_C(210);
  uint32_t DictId = UINT32_C(0);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_get");
  auto &HostFuncAVDictGet =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictGet &>(
          FuncInst->getHostFunc());

  // KeyPtr is in bounds but the guest-declared key length runs off the end of
  // linear memory; the host must reject it, not read past the page.
  uint32_t OutOfBoundsKeyPtr = UINT32_C(65000);
  uint32_t OutOfBoundsKeyLen = UINT32_C(2000);
  HostFuncAVDictGet.run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            DictId, OutOfBoundsKeyPtr, OutOfBoundsKeyLen,
                            UINT32_C(0), Flags, KeyLenPtr, ValueLenPtr},
                        Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::MissingMemory));
}

TEST_F(FFmpegTest, AVDictSetKeyLenBounds) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  int32_t Flags = 0;
  uint32_t ValuePtr = UINT32_C(4);
  uint32_t ValueLen = 5;
  uint32_t DictPtr = UINT32_C(80);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_set");
  auto &HostFuncAVDictSet =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictSet &>(
          FuncInst->getHostFunc());

  // KeyPtr is in bounds but the guest-declared key length runs off the end of
  // linear memory; the host must reject it, not read past the page.
  uint32_t OutOfBoundsKeyPtr = UINT32_C(65000);
  uint32_t OutOfBoundsKeyLen = UINT32_C(2000);
  HostFuncAVDictSet.run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            DictPtr, OutOfBoundsKeyPtr, OutOfBoundsKeyLen,
                            ValuePtr, ValueLen, Flags},
                        Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::MissingMemory));
}

TEST_F(FFmpegTest, AVDictGetLargePrevIndexTerminates) {
  using namespace std::literals::string_view_literals;
  ASSERT_TRUE(AVUtilMod != nullptr);

  uint32_t KeyStart = UINT32_C(1);
  uint32_t KeyLen = 3;
  uint32_t ValueStart = UINT32_C(4);
  uint32_t ValueLen = 5;
  int32_t Flags = 0;
  uint32_t DictPtr = UINT32_C(80);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_set");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncAVDictSet =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictSet &>(
          FuncInst->getHostFunc());

  fillMemContent(MemInst, KeyStart, "KEY"sv);
  fillMemContent(MemInst, ValueStart, "VALUE"sv);
  HostFuncAVDictSet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DictPtr, KeyStart, KeyLen,
                                                  ValueStart, ValueLen, Flags},
      Result);
  ASSERT_TRUE(readUInt32(MemInst, DictPtr) > 0);
  uint32_t DictId = readUInt32(MemInst, DictPtr);

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_get");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncAVDictGet =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictGet &>(
          FuncInst->getHostFunc());

  // A guest-supplied previous-entry index past the number of matching entries
  // must terminate and report InternalError. With UINT32_MAX an unbounded loop
  // wraps its counter back to zero and never returns, hanging the host.
  uint32_t KeyLenPtr = UINT32_C(56);
  uint32_t ValueLenPtr = UINT32_C(60);
  HostFuncAVDictGet.run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            DictId, KeyStart, KeyLen, UINT32_C(0xFFFFFFFF),
                            Flags, KeyLenPtr, ValueLenPtr},
                        Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
}

TEST_F(FFmpegTest, AVDictStaleIdHandling) {
  using namespace std::literals::string_view_literals;
  ASSERT_TRUE(AVUtilMod != nullptr);

  uint32_t KeyStart = UINT32_C(1);
  uint32_t KeyLen = 3;
  uint32_t ValueStart = UINT32_C(4);
  uint32_t ValueLen = 5;
  int32_t Flags = 0;
  uint32_t DictPtr = UINT32_C(80);
  uint32_t StaleDictId = UINT32_C(424242);

  fillMemContent(MemInst, KeyStart, "KEY"sv);
  fillMemContent(MemInst, ValueStart, "VALUE"sv);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_set");
  auto &HostFuncAVDictSet =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictSet &>(
          FuncInst->getHostFunc());

  // A nonzero id that was never allocated must be rejected instead of being
  // handed to av_dict_set as a null map.
  writeUInt32(MemInst, StaleDictId, DictPtr);
  HostFuncAVDictSet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DictPtr, KeyStart, KeyLen,
                                                  ValueStart, ValueLen, Flags},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

  writeUInt32(MemInst, UINT32_C(0), DictPtr);
  HostFuncAVDictSet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DictPtr, KeyStart, KeyLen,
                                                  ValueStart, ValueLen, Flags},
      Result);
  ASSERT_TRUE(Result[0].get<int32_t>() >= 0);
  uint32_t DictId = readUInt32(MemInst, DictPtr);
  ASSERT_TRUE(DictId > 0);

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_copy");
  auto &HostFuncAVDictCopy =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictCopy &>(
          FuncInst->getHostFunc());

  // A stale destination id must be rejected the same way.
  uint32_t DestDictPtr = UINT32_C(84);
  writeUInt32(MemInst, StaleDictId, DestDictPtr);
  HostFuncAVDictCopy.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{DestDictPtr, DictId, Flags},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_free");
  auto &HostFuncAVDictFree =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictFree &>(
          FuncInst->getHostFunc());

  // Freeing a stale id is a harmless no-op, and freeing a valid id twice must
  // not turn into a null-map free on the second call.
  HostFuncAVDictFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{StaleDictId},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  HostFuncAVDictFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DictId}, Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  HostFuncAVDictFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{DictId}, Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
