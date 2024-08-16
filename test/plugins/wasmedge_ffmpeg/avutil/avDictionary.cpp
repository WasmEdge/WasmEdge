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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDictSet =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictSet &>(
          FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDictCopy =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictCopy &>(
          FuncInst->getHostFunc());

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDictGet =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictGet &>(
          FuncInst->getHostFunc());

  {
    // Store the string length of Key and value in below Pointers.
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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDictGetKeyValue =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictGetKeyValue &>(
          FuncInst->getHostFunc());

  {
    // Store the string of Key and value in below Buffer Pointers.
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
    // Verify String. Read String from MemInst

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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDictFree =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictFree &>(
          FuncInst->getHostFunc());

  {
    uint32_t DictId = readUInt32(MemInst, DictPtr);
    EXPECT_TRUE(HostFuncAVDictFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{DictId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
