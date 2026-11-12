// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "avcodec/avPacket.h"
#include "avcodec/module.h"

#include "utils.h"

extern "C" {
#include "libavcodec/packet.h"
#include "libavutil/buffer.h"
}

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
    EXPECT_TRUE(
        HostFuncAVPacketData.run(CallFrame,
                                 std::initializer_list<WasmEdge::ValVariant>{
                                     PacketId, DataPtr, PacketDataSize},
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
  auto &HostFuncAVPacketAlloc = FuncInst->getHostFunc();
  HostFuncAVPacketAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketPtr},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t PacketId = readUInt32(MemInst, PacketPtr);
  ASSERT_TRUE(PacketId > 0);

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_new_packet");
  auto &HostFuncAVNewPacket = FuncInst->getHostFunc();
  uint32_t PacketSize = UINT32_C(40);
  HostFuncAVNewPacket.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{PacketId, PacketSize},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), 0);

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_data");
  auto &HostFuncAVPacketData = FuncInst->getHostFunc();

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

  HostFuncAVPacketData.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               PacketId, DataPtr, PacketSize},
                           Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVPacketDataEmptyPacketZeroLength) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  uint32_t PacketPtr = UINT32_C(4);
  auto *FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_alloc");
  auto &HostFuncAVPacketAlloc = FuncInst->getHostFunc();
  HostFuncAVPacketAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketPtr},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t PacketId = readUInt32(MemInst, PacketPtr);
  ASSERT_TRUE(PacketId > 0);

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_size");
  auto &HostFuncAVPacketSize = FuncInst->getHostFunc();
  HostFuncAVPacketSize.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{PacketId}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), 0);

  FuncInst =
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_data");
  auto &HostFuncAVPacketData = FuncInst->getHostFunc();

  uint32_t DataPtr = UINT32_C(200);
  uint32_t DataLen = UINT32_C(8);
  fillMemContent(MemInst, DataPtr, DataLen, UINT8_C(0xAA));
  HostFuncAVPacketData.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               PacketId, DataPtr, UINT32_C(0)},
                           Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  char *Buf = MemInst->getPointer<char *>(DataPtr);
  for (uint32_t I = 0; I < DataLen; ++I) {
    EXPECT_EQ(static_cast<uint8_t>(Buf[I]), UINT8_C(0xAA));
  }
}

TEST_F(FFmpegTest, AVPacketUnrefNullSafeAndIdempotent) {
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_unref",
                {UINT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));

  uint32_t PacketPtr = UINT32_C(4);
  allocPacket(PacketPtr);
  uint32_t PacketId = readUInt32(MemInst, PacketPtr);
  ASSERT_TRUE(PacketId > 0);

  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_unref",
                {PacketId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_unref",
                {PacketId}),
            static_cast<int32_t>(ErrNo::Success));

  uint32_t FramePtr = UINT32_C(8);
  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_unref",
                {FrameId}),
            static_cast<int32_t>(ErrNo::InternalError));

  EXPECT_EQ(
      Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
      static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVShrinkPacketRejectsNegativeSize) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t PacketPtr = UINT32_C(4);
  allocPacket(PacketPtr);
  uint32_t PacketId = readUInt32(MemInst, PacketPtr);
  ASSERT_TRUE(PacketId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_new_packet",
                {PacketId, INT32_C(100)}),
            0);

  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_shrink_packet",
                {PacketId, INT32_C(-1)}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_size",
                {PacketId}),
            100);
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_shrink_packet",
                {PacketId, INT32_C(50)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_size",
                {PacketId}),
            50);

  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_unref",
                {PacketId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVPacketRefRejectsAliasAndReplacesDestination) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVCodecMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t SrcPtr = UINT32_C(4);
  uint32_t DestPtr = UINT32_C(8);
  allocPacket(SrcPtr);
  allocPacket(DestPtr);
  uint32_t SrcId = readUInt32(MemInst, SrcPtr);
  uint32_t DestId = readUInt32(MemInst, DestPtr);
  ASSERT_TRUE(SrcId > 0);
  ASSERT_TRUE(DestId > 0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_new_packet", {SrcId, INT32_C(100)}),
            0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_new_packet", {DestId, INT32_C(16)}),
            0);

  // av_packet_ref starts by clearing dst->buf, which drops the source's own
  // reference when both are the same packet.
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_packet_ref", {SrcId, SrcId}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_packet_size", {SrcId}), 100);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_packet_ref", {DestId, SrcId}), 0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_packet_size", {DestId}), 100);

  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_packet_unref", {SrcId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_packet_unref", {DestId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVPacketSetFlagsAcceptsOnlyPublicFlags) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVCodecMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t PacketPtr = UINT32_C(4);
  allocPacket(PacketPtr);
  uint32_t PacketId = readUInt32(MemInst, PacketPtr);
  ASSERT_TRUE(PacketId > 0);

  int32_t const PublicFlags = AV_PKT_FLAG_KEY | AV_PKT_FLAG_CORRUPT |
                              AV_PKT_FLAG_DISCARD | AV_PKT_FLAG_DISPOSABLE;
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_packet_set_flags",
                {PacketId, PublicFlags}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_packet_flags", {PacketId}),
            PublicFlags);

  // AV_PKT_FLAG_TRUSTED, libavformat's internal uncoded-frame bit and any
  // other unknown bit change how FFmpeg reads the payload: rejected, and
  // the previous flags stay in place.
  for (int32_t const Flags : {static_cast<int32_t>(AV_PKT_FLAG_TRUSTED),
                              INT32_C(0x2000), INT32_C(0x40), INT32_C(-1)}) {
    EXPECT_EQ(
        Run("wasmedge_ffmpeg_avcodec_av_packet_set_flags", {PacketId, Flags}),
        static_cast<int32_t>(ErrNo::InternalError));
    EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_packet_flags", {PacketId}),
              PublicFlags);
  }

  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_packet_unref", {PacketId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVNewPacketAndGrowPacketZeroThePayload) {
  ASSERT_TRUE(AVCodecMod != nullptr);
  ASSERT_TRUE(AVFormatMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  // Demux a real packet and release it, so the allocator may hand its
  // block of non-zero media data back to a fresh packet of the same size
  // (whether it does depends on the allocator; the zero-fill contract must
  // hold either way).
  uint32_t FormatCtxPtr = UINT32_C(4);
  uint32_t PacketPtr = UINT32_C(8);
  uint32_t FilePtr = UINT32_C(100);
  initFormatCtx(FormatCtxPtr, FilePtr,
                std::string("ffmpeg-assets/sample_video.mp4"));
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  ASSERT_TRUE(FormatCtxId > 0);
  int32_t Size = 0;
  uint32_t PacketId = 0;
  for (int Attempt = 0; Attempt < 64; ++Attempt) {
    allocPacket(PacketPtr);
    PacketId = readUInt32(MemInst, PacketPtr);
    ASSERT_TRUE(PacketId > 0);
    if (Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_av_read_frame",
            {FormatCtxId, PacketId}) >= 0) {
      Size = Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_size",
                 {PacketId});
    }
    EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_unref",
                  {PacketId}),
              static_cast<int32_t>(ErrNo::Success));
    if (Size >= 4096 && Size <= 16384) {
      break;
    }
    Size = 0;
  }
  if (Size == 0) {
    Size = 8192;
  }

  uint32_t const BufPtr = UINT32_C(1024);
  uint32_t const Grow = UINT32_C(4096);
  uint32_t const Total = static_cast<uint32_t>(Size) + Grow;
  allocPacket(PacketPtr);
  PacketId = readUInt32(MemInst, PacketPtr);
  ASSERT_TRUE(PacketId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_new_packet",
                {PacketId, Size}),
            0);
  fillMemContent(MemInst, BufPtr, Total, UINT8_C(0xAA));
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_data",
                {PacketId, BufPtr, static_cast<uint32_t>(Size)}),
            static_cast<int32_t>(ErrNo::Success));
  const uint8_t *Buf = MemInst->getPointer<const uint8_t *>(BufPtr);
  EXPECT_EQ(std::count(Buf, Buf + Size, UINT8_C(0)),
            static_cast<std::ptrdiff_t>(Size));

  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_grow_packet",
                {PacketId, static_cast<int32_t>(Grow)}),
            0);
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_size",
                {PacketId}),
            static_cast<int32_t>(Total));
  fillMemContent(MemInst, BufPtr, Total, UINT8_C(0xAA));
  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_data",
                {PacketId, BufPtr, Total}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(std::count(Buf, Buf + Total, UINT8_C(0)),
            static_cast<std::ptrdiff_t>(Total));

  EXPECT_EQ(Run(AVCodecMod.get(), "wasmedge_ffmpeg_avcodec_av_packet_unref",
                {PacketId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_close_input", {FormatCtxId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVNewPacketReleasesThePreviousPayload) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVCodecMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  auto &HostFuncAVPacketAlloc = dynamic_cast<AVcodec::AVPacketAlloc &>(
      AVCodecMod->findFuncExports("wasmedge_ffmpeg_avcodec_av_packet_alloc")
          ->getHostFunc());
  auto Env = HostFuncAVPacketAlloc.getEnv();
  auto BufferRefs = [&](uint32_t PacketId) {
    auto *Packet = static_cast<AVPacket *>(Env->fetchData(PacketId));
    if (Packet == nullptr || Packet->buf == nullptr) {
      return 0;
    }
    return av_buffer_get_ref_count(Packet->buf);
  };

  uint32_t PacketPtr = UINT32_C(4);
  uint32_t CopyPtr = UINT32_C(8);
  allocPacket(PacketPtr);
  allocPacket(CopyPtr);
  uint32_t PacketId = readUInt32(MemInst, PacketPtr);
  uint32_t CopyId = readUInt32(MemInst, CopyPtr);
  ASSERT_TRUE(PacketId > 0);
  ASSERT_TRUE(CopyId > 0);

  // A second reference to the first payload shows whether the packet let
  // go of it when it was given a new one.
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avcodec_av_new_packet", {PacketId, INT32_C(64)}), 0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_packet_ref", {CopyId, PacketId}),
            0);
  EXPECT_EQ(BufferRefs(CopyId), 2);
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avcodec_av_new_packet", {PacketId, INT32_C(32)}), 0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_packet_size", {PacketId}), 32);
  EXPECT_EQ(BufferRefs(CopyId), 1);

  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_packet_unref", {PacketId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avcodec_av_packet_unref", {CopyId}),
            static_cast<int32_t>(ErrNo::Success));
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
