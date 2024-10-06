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
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVPacketAlloc =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketAlloc &>(
          FuncInst->getHostFunc());

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

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_new_packet");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVNewPacket =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVNewPacket &>(
          FuncInst->getHostFunc());

  {
    uint32_t Size = 40;
    EXPECT_TRUE(HostFuncAVNewPacket.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId, Size},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_grow_packet");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVGrowPacket =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVGrowPacket &>(
          FuncInst->getHostFunc());

  {
    uint32_t Size = 40;
    EXPECT_TRUE(HostFuncAVGrowPacket.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId, Size},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_shrink_packet");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVShrinkPacket =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVShrinkPacket &>(
          FuncInst->getHostFunc());

  {
    uint32_t Size = 40;
    EXPECT_TRUE(HostFuncAVShrinkPacket.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId, Size},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  uint32_t StreamIdx = 3;
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_packet_set_stream_index");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketSetStreamIndex = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketSetStreamIndex &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketSetStreamIndex.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{PacketId, StreamIdx},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_packet_stream_index");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketStreamIndex = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketStreamIndex &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketStreamIndex.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), StreamIdx);
  }

  uint32_t Size = 0;
  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_size");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketSize =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketSize &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketSize.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    Size = Result[0].get<int32_t>();
    EXPECT_TRUE(Size > 0);
  }

  uint32_t Flags = 5;

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_packet_set_flags");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketSetFlags =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketSetFlags &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketSetFlags.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId, Flags},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_flags");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketFlags =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketFlags &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketFlags.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), Flags);
  }

  int64_t Pos = 500;
  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_set_pos");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketSetPos =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketSetPos &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketSetPos.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId, Pos},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_pos");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketPos =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketPos &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketPos.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), Pos);
  }

  int64_t Duration = 100;
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_packet_set_duration");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketSetDuration = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketSetDuration &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketSetDuration.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{PacketId, Duration},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_duration");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketDuration =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketDuration &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketDuration.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), Duration);
  }

  int64_t Dts = 1000;
  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_set_dts");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketSetDts =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketSetDts &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketSetDts.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId, Dts},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_dts");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketDts =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketDts &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketDts.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), Dts);
  }

  int64_t Pts = 5000;
  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_set_pts");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketSetPts =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketSetPts &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketSetPts.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId, Pts},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_pts");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketPts =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketPts &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketPts.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int64_t>(), Pts);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_av_packet_is_data_null");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketIsDataNull = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketIsDataNull &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketIsDataNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_data");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVPacketData =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketData &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketData.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{PacketId, DataPtr, Size},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_ref");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVPacketRef =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketRef &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketRef.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{PacketId2, PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_unref");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVPacketUnref =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVPacketUnref &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVPacketUnref.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
