// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVDeviceListDevicesFailureZeroesTheIdSlot) {
  ASSERT_TRUE(AVDeviceMod != nullptr);

  auto *FuncInst = AVDeviceMod->findFuncExports(
      "wasmedge_ffmpeg_avdevice_avdevice_list_devices");
  ASSERT_NE(FuncInst, nullptr);
  uint32_t ListPtr = UINT32_C(4);
  writeUInt32(MemInst, UINT32_C(0xABCD), ListPtr);
  EXPECT_TRUE(FuncInst->getHostFunc().run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0xDEAD), ListPtr},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(readUInt32(MemInst, ListPtr), UINT32_C(0));
}

TEST_F(FFmpegTest, AVDeviceFreeListDevicesNullSafeAndIdempotent) {
  ASSERT_TRUE(AVDeviceMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  EXPECT_EQ(Run(AVDeviceMod.get(),
                "wasmedge_ffmpeg_avdevice_avdevice_free_list_devices",
                {UINT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVDeviceMod.get(),
                "wasmedge_ffmpeg_avdevice_avdevice_free_list_devices",
                {UINT32_C(999999)}),
            static_cast<int32_t>(ErrNo::Success));

  uint32_t FramePtr = UINT32_C(4);
  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);
  EXPECT_EQ(Run(AVDeviceMod.get(),
                "wasmedge_ffmpeg_avdevice_avdevice_free_list_devices",
                {FrameId}),
            static_cast<int32_t>(ErrNo::InternalError));

  EXPECT_EQ(
      Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
      static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVDeviceListDevicesFailureStoresNothing) {
  ASSERT_TRUE(AVDeviceMod != nullptr);
  ASSERT_TRUE(AVFormatMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t FormatCtxPtr = UINT32_C(4);
  uint32_t ListPtr = UINT32_C(8);
  uint32_t FilePtr = UINT32_C(100);
  initFormatCtx(FormatCtxPtr, FilePtr,
                std::string("ffmpeg-assets/sample_video.mp4"));
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  ASSERT_TRUE(FormatCtxId > 0);

  writeUInt32(MemInst, UINT32_C(0xABCD), ListPtr);
  EXPECT_LT(Run(AVDeviceMod.get(),
                "wasmedge_ffmpeg_avdevice_avdevice_list_devices",
                {FormatCtxId, ListPtr}),
            0);
  EXPECT_EQ(readUInt32(MemInst, ListPtr), UINT32_C(0));
  EXPECT_EQ(Run(AVDeviceMod.get(),
                "wasmedge_ffmpeg_avdevice_avdevice_nb_devices",
                {UINT32_C(0xABCD)}),
            0);

  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_close_input", {FormatCtxId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVDeviceListDevicesRejectsAFinishedMuxer) {
  ASSERT_TRUE(AVDeviceMod != nullptr);
  ASSERT_TRUE(AVFormatMod != nullptr);
  ASSERT_TRUE(AVCodecMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  uint32_t CodecPtr = UINT32_C(4);
  uint32_t EncCtxPtr = UINT32_C(8);
  uint32_t CtxPtr = UINT32_C(12);
  uint32_t ParPtr = UINT32_C(16);
  uint32_t ListPtr = UINT32_C(20);
  uint32_t FormatPtr = UINT32_C(100);
  uint32_t FilePtr = UINT32_C(200);
  std::string Format("matroska");
  std::string File("device_list_after_trailer.mkv");
  fillMemContent(MemInst, FormatPtr, Format);
  fillMemContent(MemInst, FilePtr, File);

  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_find_encoder",
                {UINT32_C(216), CodecPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t CodecId = readUInt32(MemInst, CodecPtr);
  ASSERT_TRUE(CodecId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_alloc_context3",
                {CodecId, EncCtxPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t EncCtxId = readUInt32(MemInst, EncCtxPtr);
  ASSERT_TRUE(EncCtxId > 0);
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_sample_format",
                {EncCtxId, UINT32_C(2)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_sample_rate",
                {EncCtxId, INT32_C(48000)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodeccontext_set_channel_layout",
                {EncCtxId, UINT64_C(3)}),
            static_cast<int32_t>(ErrNo::Success));

  EXPECT_GE(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_alloc_output_context2",
                {CtxPtr, UINT32_C(0), FormatPtr,
                 static_cast<uint32_t>(Format.length()), FilePtr,
                 static_cast<uint32_t>(File.length())}),
            0);
  uint32_t CtxId = readUInt32(MemInst, CtxPtr);
  ASSERT_TRUE(CtxId > 0);
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_new_stream",
                {CtxId, UINT32_C(0)}),
            1);
  EXPECT_EQ(Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avStream_codecpar",
                {CtxId, UINT32_C(0), ParPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t ParId = readUInt32(MemInst, ParPtr);
  ASSERT_TRUE(ParId > 0);
  EXPECT_GE(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_parameters_from_context",
                {ParId, EncCtxId}),
            0);
  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avStream_set_timebase",
                {UINT32_C(1), UINT32_C(48000), CtxId, UINT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_GE(
      Run(AVFormatMod.get(), "wasmedge_ffmpeg_avformat_avio_open",
          {CtxId, FilePtr, static_cast<uint32_t>(File.length()), INT32_C(2)}),
      0);
  EXPECT_GE(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_write_header",
                {CtxId, UINT32_C(0)}),
            0);

  // A live muxer reaches libavdevice, which has no list for matroska.
  EXPECT_LT(Run(AVDeviceMod.get(),
                "wasmedge_ffmpeg_avdevice_avdevice_list_devices",
                {CtxId, ListPtr}),
            0);
  EXPECT_NE(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

  // av_write_trailer frees priv_data, so the muxer may not be queried after.
  EXPECT_GE(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_write_trailer", {CtxId}),
            0);
  writeUInt32(MemInst, UINT32_C(0xABCD), ListPtr);
  EXPECT_EQ(Run(AVDeviceMod.get(),
                "wasmedge_ffmpeg_avdevice_avdevice_list_devices",
                {CtxId, ListPtr}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(readUInt32(MemInst, ListPtr), UINT32_C(0));

  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_free_context", {CtxId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVCodecMod.get(),
                "wasmedge_ffmpeg_avcodec_avcodec_free_context", {EncCtxId}),
            static_cast<int32_t>(ErrNo::Success));
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
