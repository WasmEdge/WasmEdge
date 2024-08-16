// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avcodec/avCodecContext.h"
#include "avcodec/module.h"

#include "utils.h"

#include <gtest/gtest.h>

// Testing all AVCodecCtxstruct
namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVCodecCtx) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  uint32_t AVCodecCtxPtr = UINT32_C(64);

  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  initFFmpegStructs(UINT32_C(20), UINT32_C(24), UINT32_C(28), FileName,
                    UINT32_C(60), AVCodecCtxPtr, UINT32_C(68), UINT32_C(72));
  uint32_t NumPtr = UINT32_C(76);
  uint32_t DenPtr = UINT32_C(80);
  uint32_t AVCodecPtr = UINT32_C(84);

  uint32_t AVCodecCtxId = readUInt32(MemInst, AVCodecCtxPtr);

  auto *FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_codec_id");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxCodecID = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxCodecID &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxCodecID.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<uint32_t>(), 27); // H264
  }

  int32_t CodecType = 0; // MediaType Video
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_codec_type");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetCodecType = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetCodecType &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetCodecType.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, CodecType},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_codec_type");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxCodecType = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxCodecType &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxCodecType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), CodecType); // MediaType Video
  }

  int32_t Num = 5;
  int32_t Den = 10;
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_time_base");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetTimebase = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetTimebase &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetTimebase.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, Num, Den},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_time_base");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxTimeBase = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxTimeBase &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxTimeBase.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, NumPtr,
                                                    DenPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    int32_t Numerator = readSInt32(MemInst, NumPtr);
    int32_t Denominator = readSInt32(MemInst, DenPtr);
    EXPECT_EQ(Numerator, Num);
    EXPECT_EQ(Denominator, Den);
  }

  int32_t Dimension = 200;
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_width");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetWidth = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetWidth &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetWidth.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, Dimension},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_width");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxWidth =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxWidth &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxWidth.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), Dimension);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_height");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetHeight = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetHeight &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetHeight.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, Dimension},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_height");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxHeight =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxHeight &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxHeight.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), Dimension);
  }

  Num = 10;
  Den = 20;
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_sample_aspect_ratio");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetSampleAspectRatio = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetSampleAspectRatio
          &>(FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetSampleAspectRatio.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, Num, Den},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_sample_aspect_ratio");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSampleAspectRatio = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSampleAspectRatio &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSampleAspectRatio.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, NumPtr,
                                                    DenPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    int32_t Numerator = readSInt32(MemInst, NumPtr);
    int32_t Denominator = readSInt32(MemInst, DenPtr);
    EXPECT_EQ(Numerator, Num);
    EXPECT_EQ(Denominator, Den);
  }

  uint64_t ChannelLayoutId = 1; // FRONT_LEFT;
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_channel_layout");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetChannelLayout = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetChannelLayout &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetChannelLayout.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    ChannelLayoutId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_channel_layout");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxChannelLayout = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxChannelLayout &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxChannelLayout.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<uint64_t>(), ChannelLayoutId);
  }

  uint32_t PixFormatId = 1; // YUV420P

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_pix_fmt");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetPixFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetPixFormat &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetPixFormat.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, PixFormatId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_pix_fmt");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxPixFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxPixFormat &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxPixFormat.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), PixFormatId);
  }

  uint32_t SampleFmtId = 1; // SAMPLE_FMT_U8
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_sample_format");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetSampleFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetSampleFormat &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetSampleFormat.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, SampleFmtId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_sample_format");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSampleFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSampleFormat &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSampleFormat.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), SampleFmtId);
  }

  int32_t SampleRate = 500;
  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_sample_rate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetSampleRate = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetSampleRate &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetSampleRate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, SampleRate},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_sample_rate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSampleRate = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSampleRate &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSampleRate.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), SampleRate);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_gop_size");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetGopSize = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetGopSize &>(
      FuncInst->getHostFunc());

  {
    int32_t GopSize = 20;
    EXPECT_TRUE(HostFuncAVCodecCtxSetGopSize.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, GopSize},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_max_b_frames");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetMaxBFrames = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetMaxBFrames &>(
      FuncInst->getHostFunc());

  {
    int32_t MaxBFrames = 30;
    EXPECT_TRUE(HostFuncAVCodecCtxSetMaxBFrames.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, MaxBFrames},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_b_quant_factor");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetBQuantFactor = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetBQuantFactor &>(
      FuncInst->getHostFunc());

  {
    float BQuantFactor = 12.32;
    EXPECT_TRUE(HostFuncAVCodecCtxSetBQuantFactor.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, BQuantFactor},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_b_quant_offset");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetBQuantOffset = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetBQuantOffset &>(
      FuncInst->getHostFunc());

  {
    float BQuantOffset = 3.53;
    EXPECT_TRUE(HostFuncAVCodecCtxSetBQuantOffset.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, BQuantOffset},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_i_quant_factor");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetIQuantFactor = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetIQuantFactor &>(
      FuncInst->getHostFunc());

  {
    float IQuantFactor = 3.435;
    EXPECT_TRUE(HostFuncAVCodecCtxSetIQuantFactor.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, IQuantFactor},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_i_quant_offset");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetIQuantOffset = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetIQuantOffset &>(
      FuncInst->getHostFunc());

  {
    float IQuantOffset = 6.322;
    EXPECT_TRUE(HostFuncAVCodecCtxSetIQuantOffset.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, IQuantOffset},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_lumi_masking");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetLumiMasking = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetLumiMasking &>(
      FuncInst->getHostFunc());

  {
    float LumiMasking = 54.32432;
    EXPECT_TRUE(HostFuncAVCodecCtxSetLumiMasking.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, LumiMasking},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_temporal_cplx_masking");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetTemporalCplxMasking = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetTemporalCplxMasking
          &>(FuncInst->getHostFunc());

  {
    float TemporialCplxMasking = 642.32;
    EXPECT_TRUE(HostFuncAVCodecCtxSetTemporalCplxMasking.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    TemporialCplxMasking},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_spatial_cplx_masking");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetSpatialCplxMasking = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetSpatialCplxMasking
          &>(FuncInst->getHostFunc());

  {
    float SpatialCplxMasking = 324.32;
    EXPECT_TRUE(HostFuncAVCodecCtxSetSpatialCplxMasking.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    SpatialCplxMasking},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_p_masking");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetPMasking = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetPMasking &>(
      FuncInst->getHostFunc());

  {
    float PMasking = 65.3245;
    EXPECT_TRUE(HostFuncAVCodecCtxSetPMasking.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, PMasking},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_dark_masking");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetDarkMasking = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetDarkMasking &>(
      FuncInst->getHostFunc());

  {
    float DarkMasking = 83.32;
    EXPECT_TRUE(HostFuncAVCodecCtxSetDarkMasking.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, DarkMasking},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_me_cmp");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetMeCmp = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetMeCmp &>(
      FuncInst->getHostFunc());

  {
    int32_t MeCmp = 532;
    EXPECT_TRUE(HostFuncAVCodecCtxSetMeCmp.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, MeCmp},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_me_sub_cmp");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetMeSubCmp = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetMeSubCmp &>(
      FuncInst->getHostFunc());

  {
    int32_t MeSubCmp = 321;
    EXPECT_TRUE(HostFuncAVCodecCtxSetMeSubCmp.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, MeSubCmp},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_mb_cmp");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetMbCmp = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetMbCmp &>(
      FuncInst->getHostFunc());

  {
    int32_t MbCmp = 243;
    EXPECT_TRUE(HostFuncAVCodecCtxSetMbCmp.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, MbCmp},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_ildct_cmp");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetIldctCmp = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetIldctCmp &>(
      FuncInst->getHostFunc());

  {
    int32_t IldctCmp = 3;
    EXPECT_TRUE(HostFuncAVCodecCtxSetIldctCmp.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, IldctCmp},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_dia_size");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetDiaSize = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetDiaSize &>(
      FuncInst->getHostFunc());

  {
    int32_t DiaSize = 9;
    EXPECT_TRUE(HostFuncAVCodecCtxSetDiaSize.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, DiaSize},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_last_predictor_count");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetLastPredictorsCount = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetLastPredictorsCount
          &>(FuncInst->getHostFunc());

  {
    int32_t LastPredictorCount = 21;
    EXPECT_TRUE(HostFuncAVCodecCtxSetLastPredictorsCount.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    LastPredictorCount},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_me_pre_cmp");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetMePreCmp = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetMePreCmp &>(
      FuncInst->getHostFunc());

  {
    int32_t MePreCmp = 53;
    EXPECT_TRUE(HostFuncAVCodecCtxSetMePreCmp.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, MePreCmp},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_pre_dia_size");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetPreDiaSize = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetPreDiaSize &>(
      FuncInst->getHostFunc());

  {
    int32_t PreDiaSize = 74;
    EXPECT_TRUE(HostFuncAVCodecCtxSetPreDiaSize.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, PreDiaSize},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_me_subpel_quality");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetMeSubpelQuality = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetMeSubpelQuality &>(
      FuncInst->getHostFunc());

  {
    int32_t MeSubpelQuality = 85;
    EXPECT_TRUE(HostFuncAVCodecCtxSetMeSubpelQuality.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    MeSubpelQuality},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_me_range");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetMeRange = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetMeRange &>(
      FuncInst->getHostFunc());

  {
    int32_t SetMeRange = 31;
    EXPECT_TRUE(HostFuncAVCodecCtxSetMeRange.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, SetMeRange},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_mb_decision");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetMbDecision = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetMbDecision &>(
      FuncInst->getHostFunc());

  {
    int32_t MbDecision = 78;
    EXPECT_TRUE(HostFuncAVCodecCtxSetMbDecision.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, MbDecision},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_mb_lmin");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetMbLMin = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetMbLMin &>(
      FuncInst->getHostFunc());

  {
    int32_t MbLMin = 11;
    EXPECT_TRUE(HostFuncAVCodecCtxSetMbLMin.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, MbLMin},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_mb_lmax");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetMbLMax = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetMbLMax &>(
      FuncInst->getHostFunc());

  {
    int32_t MbLMax = 18;
    EXPECT_TRUE(HostFuncAVCodecCtxSetMbLMax.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, MbLMax},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_intra_dc_precision");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  int32_t IntraDcPrecision = 323;
  auto &HostFuncAVCodecCtxSetIntraDcPrecision = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetIntraDcPrecision &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetIntraDcPrecision.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    IntraDcPrecision},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_intra_dc_precision");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxIntraDcPrecision = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxIntraDcPrecision &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxIntraDcPrecision.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), IntraDcPrecision);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_qmin");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetQMin = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetQMin &>(
      FuncInst->getHostFunc());

  {
    int32_t QMin = 10;
    EXPECT_TRUE(HostFuncAVCodecCtxSetQMin.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, QMin},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_qmax");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetQMax = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetQMax &>(
      FuncInst->getHostFunc());

  {
    int32_t QMax = 20;
    EXPECT_TRUE(HostFuncAVCodecCtxSetQMax.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, QMax},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_global_quality");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetGlobalQuality = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetGlobalQuality &>(
      FuncInst->getHostFunc());

  {
    int32_t GlobalQuality = 93;
    EXPECT_TRUE(HostFuncAVCodecCtxSetGlobalQuality.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    GlobalQuality},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_colorspace");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetColorspace = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetColorspace &>(
      FuncInst->getHostFunc());

  int32_t ColorspaceId = 1; // AVCOL_SPC_BT709
  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetColorspace.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, ColorspaceId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_colorspace");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxColorspace = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxColorspace &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxColorspace.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), ColorspaceId);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_color_range");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetColorRange = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetColorRange &>(
      FuncInst->getHostFunc());

  int32_t ColorRangeId = 1; // MPEG
  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetColorRange.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, ColorRangeId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_color_range");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxColorRange = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxColorRange &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxColorRange.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), ColorRangeId);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_frame_size");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxFrameSize = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxFrameSize &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxFrameSize.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_bit_rate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetBitRate = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetBitRate &>(
      FuncInst->getHostFunc());

  int64_t BitRate = 9932;
  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetBitRate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, BitRate},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_bit_rate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxBitRate = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxBitRate &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxBitRate.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), BitRate);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_rc_max_rate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  int64_t RcMaxRate = 3245;
  auto &HostFuncAVCodecCtxSetRcMaxRate = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetRcMaxRate &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetRcMaxRate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, RcMaxRate},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_rc_max_rate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxRcMaxRate = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxRcMaxRate &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxRcMaxRate.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), RcMaxRate);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_bit_rate_tolerance");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetBitRateTolerance = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetBitRateTolerance &>(
      FuncInst->getHostFunc());

  {
    int32_t BitRateTolerance = 9543;
    EXPECT_TRUE(HostFuncAVCodecCtxSetBitRateTolerance.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    BitRateTolerance},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_compression_level");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetCompressionLevel = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetCompressionLevel &>(
      FuncInst->getHostFunc());

  {
    int32_t CompressionLevel = 934;
    EXPECT_TRUE(HostFuncAVCodecCtxSetCompressionLevel.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    CompressionLevel},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_framerate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  Num = 20;
  Den = 30;
  auto &HostFuncAVCodecCtxSetFrameRate = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetFrameRate &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetFrameRate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, Num, Den},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_framerate");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxFrameRate = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxFrameRate &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxFrameRate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, NumPtr,
                                                    DenPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    int32_t Numerator = readSInt32(MemInst, NumPtr);
    int32_t Denominator = readSInt32(MemInst, DenPtr);
    EXPECT_EQ(Numerator, Num);
    EXPECT_EQ(Denominator, Den);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_flags");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetFlags = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetFlags &>(
      FuncInst->getHostFunc());

  {
    int32_t Flags = 3;
    EXPECT_TRUE(HostFuncAVCodecCtxSetFlags.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, Flags},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_strict_std_compliance");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetStrictStdCompliance = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetStrictStdCompliance
          &>(FuncInst->getHostFunc());

  {
    int32_t ComplianceId = 3;
    EXPECT_TRUE(HostFuncAVCodecCtxSetStrictStdCompliance.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, ComplianceId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_debug");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetDebug = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetDebug &>(
      FuncInst->getHostFunc());

  {
    int32_t Debug = 50;
    EXPECT_TRUE(HostFuncAVCodecCtxSetDebug.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, Debug},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_codec");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxCodec =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxCodec &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxCodec.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, AVCodecPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, AVCodecPtr) > 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_channels");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetChannels = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetChannels &>(
      FuncInst->getHostFunc());

  int32_t Channels = 10;
  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetChannels.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, Channels},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_channels");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxChannels = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxChannels &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxChannels.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), Channels);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_skip_loop_filter");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetSkipLoopFilter = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetSkipLoopFilter &>(
      FuncInst->getHostFunc());

  int32_t DiscardId = 16; // Bidirectional
  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetSkipLoopFilter.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, DiscardId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_skip_frame");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetSkipFrame = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetSkipFrame &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetSkipFrame.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, DiscardId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_skip_idct");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetSkipIdct = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetSkipIdct &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetSkipIdct.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, DiscardId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_error_concealment");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetErrorConcealment = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetErrorConcealment &>(
      FuncInst->getHostFunc());

  {
    int32_t ErrorConcealment = 99;
    EXPECT_TRUE(HostFuncAVCodecCtxSetErrorConcealment.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    ErrorConcealment},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_err_recognition");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetErrorRecognition = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetErrorRecognition &>(
      FuncInst->getHostFunc());

  {
    int32_t ErrorRecognition = 88;
    EXPECT_TRUE(HostFuncAVCodecCtxSetErrorRecognition.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    ErrorRecognition},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_delay");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxDelay =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxDelay &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxDelay.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_skip_top");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetSkipTop = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetSkipTop &>(
      FuncInst->getHostFunc());

  {
    int32_t Value = 50;
    EXPECT_TRUE(HostFuncAVCodecCtxSetSkipTop.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, Value},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_skip_bottom");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetSkipBottom = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetSkipBottom &>(
      FuncInst->getHostFunc());

  {
    int32_t Value = 60;
    EXPECT_TRUE(HostFuncAVCodecCtxSetSkipBottom.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, Value},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_refs");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxRefs =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxRefs &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxRefs.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 4);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_slice_flags");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetSliceFlags = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetSliceFlags &>(
      FuncInst->getHostFunc());

  {
    int32_t Value = 70;
    EXPECT_TRUE(HostFuncAVCodecCtxSetSliceFlags.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, Value},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_slice_count");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetSliceCount = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetSliceCount &>(
      FuncInst->getHostFunc());

  {
    int32_t Value = 100;
    EXPECT_TRUE(HostFuncAVCodecCtxSetSliceCount.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, Value},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_field_order");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetFieldOrder = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetFieldOrder &>(
      FuncInst->getHostFunc());

  {
    int32_t Value = 200;
    EXPECT_TRUE(HostFuncAVCodecCtxSetFieldOrder.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, Value},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_color_trc");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxColorTrc = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxColorTrc &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxColorTrc.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() > 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_chroma_sample_location");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxChromaSampleLocation = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxChromaSampleLocation
          &>(FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxChromaSampleLocation.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_frame_number");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxFrameNumber = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxFrameNumber &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxFrameNumber.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_block_align");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxBlockAlign = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxBlockAlign &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxBlockAlign.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_request_sample_fmt");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetRequestSampleFmt = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetRequestSampleFmt &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetRequestSampleFmt.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, SampleFmtId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_audio_service_type");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxAudioServiceType = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxAudioServiceType &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxAudioServiceType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_has_b_frames");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxHasBFrames = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxHasBFrames &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxHasBFrames.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() > 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_request_channel_layout");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetRequestChannelLayout = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetRequestChannelLayout
          &>(FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetRequestChannelLayout.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId,
                                                    ChannelLayoutId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_active_thread_type");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxActiveThreadType = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxActiveThreadType &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxActiveThreadType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_thread_type");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetThreadType = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetThreadType &>(
      FuncInst->getHostFunc());

  {
    int32_t ThreadType = 1; // Frame
    EXPECT_TRUE(HostFuncAVCodecCtxSetThreadType.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, ThreadType},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_set_thread_count");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxSetThreadCount = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxSetThreadCount &>(
      FuncInst->getHostFunc());

  int32_t ThreadCount = 50;
  {
    EXPECT_TRUE(HostFuncAVCodecCtxSetThreadCount.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId, ThreadCount},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_thread_count");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxThreadCount = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxThreadCount &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxThreadCount.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), ThreadCount);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_color_primaries");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxColorPrimaries = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecCtxColorPrimaries &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecCtxColorPrimaries.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() >= 0);
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
