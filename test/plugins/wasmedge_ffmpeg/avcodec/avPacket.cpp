// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avcodec/avPacket.h"
#include "avcodec/module.h"

#include "utils.h"

#include <gtest/gtest.h>

// Testing all AVPacket

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVPacketTest) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  uint32_t PacketPtr = UINT32_C(4);
  uint32_t PacketPtr2 = UINT32_C(8);
  uint32_t DataPtr = UINT32_C(12);

  auto *FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_alloc");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVPacketAlloc = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketAlloc.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_TRUE(HostFuncAVPacketAlloc.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketPtr2},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t PacketId = readUInt32(MemInst, PacketPtr);
  uint32_t PacketId2 = readUInt32(MemInst, PacketPtr2);
  ASSERT_TRUE(PacketId > 0);
  ASSERT_TRUE(PacketId2 > 0);

  uint32_t PacketDataSize = 0;
  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_size");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketSize = FuncInst->getHostFunc();
  auto ExpectPacketSize = [&](uint32_t ExpectedSize) {
    EXPECT_TRUE(HostFuncAVPacketSize.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    PacketDataSize = static_cast<uint32_t>(Result[0].get<int32_t>());
    EXPECT_EQ(PacketDataSize, ExpectedSize);
  };

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_new_packet");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVNewPacket = FuncInst->getHostFunc();

  {
    uint32_t Size = 40;
    EXPECT_TRUE(HostFuncAVNewPacket.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId, Size},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
    ExpectPacketSize(Size);
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_grow_packet");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVGrowPacket = FuncInst->getHostFunc();

  {
    uint32_t Size = 40;
    EXPECT_TRUE(HostFuncAVGrowPacket.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId, Size},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
    ExpectPacketSize(80);
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_shrink_packet");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVShrinkPacket = FuncInst->getHostFunc();

  {
    uint32_t Size = 40;
    EXPECT_TRUE(HostFuncAVShrinkPacket.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId, Size},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
    ExpectPacketSize(Size);
  }

  uint32_t StreamIdx = 3;
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_packet_set_stream_index");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketSetStreamIndex = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketSetStreamIndex.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{PacketId, StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_packet_stream_index");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketStreamIndex = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketStreamIndex.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), StreamIdx);
  }

  uint32_t Flags = 5;

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_packet_set_flags");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketSetFlags = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketSetFlags.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId, Flags},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_flags");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketFlags = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketFlags.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), Flags);
  }

  int64_t Pos = 500;
  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_set_pos");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketSetPos = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketSetPos.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId, Pos},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_pos");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketPos = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketPos.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), Pos);
  }

  int64_t Duration = 100;
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_packet_set_duration");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketSetDuration = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketSetDuration.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{PacketId, Duration},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_duration");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketDuration = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketDuration.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), Duration);
  }

  int64_t Dts = 1000;
  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_set_dts");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketSetDts = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketSetDts.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId, Dts},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_dts");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketDts = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketDts.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), Dts);
  }

  int64_t Pts = 5000;
  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_set_pts");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketSetPts = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketSetPts.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId, Pts},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_pts");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketPts = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketPts.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), Pts);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_packet_is_data_null");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketIsDataNull = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketIsDataNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_data");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketData = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketData.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{PacketId, DataPtr,
                                                    PacketDataSize},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_ref");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVPacketRef = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketRef.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{PacketId2, PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_unref");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVPacketUnref = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVPacketUnref.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}
TEST_F(FFmpegTest, AVPacketDataBounds) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  uint32_t PacketPtr = UINT32_C(4);
  auto *FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_alloc");
  auto &HostFuncAVPacketAlloc =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketAlloc &>(
          FuncInst->getHostFunc());
  HostFuncAVPacketAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketPtr},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t PacketId = readUInt32(MemInst, PacketPtr);
  ASSERT_TRUE(PacketId > 0);

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_new_packet");
  auto &HostFuncAVNewPacket =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVNewPacket &>(
          FuncInst->getHostFunc());
  uint32_t PacketSize = UINT32_C(40);
  HostFuncAVNewPacket.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PacketId, PacketSize},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), 0);

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_data");
  auto &HostFuncAVPacketData =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketData &>(
          FuncInst->getHostFunc());

  // The destination buffer is larger than the packet data and is fenced with a
  // sentinel; a guest length larger than AvPacket->size must be rejected
  // outright, since a partial copy reported as success would let the guest
  // consume the stale tail of its buffer as packet data.
  uint32_t DataPtr = UINT32_C(200);
  uint32_t DataLen = PacketSize + UINT32_C(24);
  fillMemContent(MemInst, DataPtr, DataLen, UINT8_C(0xAA));
  HostFuncAVPacketData.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PacketId, DataPtr, DataLen},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

  char *Buf = MemInst->getPointer<char *>(DataPtr);
  for (uint32_t I = 0; I < DataLen; ++I) {
    EXPECT_EQ(static_cast<uint8_t>(Buf[I]), UINT8_C(0xAA));
  }

  // A request bounded by the packet size still succeeds.
  HostFuncAVPacketData.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               PacketId, DataPtr, PacketSize},
                           Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
