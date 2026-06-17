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

  auto &HostFuncAVCodecCtxCodecID = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetCodecType = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxCodecType = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetTimebase = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxTimeBase = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetWidth = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxWidth = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetHeight = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxHeight = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetSampleAspectRatio = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSampleAspectRatio = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetChannelLayout = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxChannelLayout = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetPixFormat = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxPixFormat = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetSampleFormat = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSampleFormat = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetSampleRate = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSampleRate = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetGopSize = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetMaxBFrames = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetBQuantFactor = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetBQuantOffset = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetIQuantFactor = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetIQuantOffset = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetLumiMasking = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetTemporalCplxMasking = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetSpatialCplxMasking = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetPMasking = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetDarkMasking = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetMeCmp = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetMeSubCmp = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetMbCmp = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetIldctCmp = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetDiaSize = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetLastPredictorsCount = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetMePreCmp = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetPreDiaSize = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetMeSubpelQuality = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetMeRange = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetMbDecision = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetMbLMin = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetMbLMax = FuncInst->getHostFunc();

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
  auto &HostFuncAVCodecCtxSetIntraDcPrecision = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxIntraDcPrecision = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetQMin = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetQMax = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetGlobalQuality = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetColorspace = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxColorspace = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetColorRange = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxColorRange = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxFrameSize = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetBitRate = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxBitRate = FuncInst->getHostFunc();

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
  auto &HostFuncAVCodecCtxSetRcMaxRate = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxRcMaxRate = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetBitRateTolerance = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetCompressionLevel = FuncInst->getHostFunc();

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
  auto &HostFuncAVCodecCtxSetFrameRate = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxFrameRate = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetFlags = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetStrictStdCompliance = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetDebug = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxCodec = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetChannels = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxChannels = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetSkipLoopFilter = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetSkipFrame = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetSkipIdct = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetErrorConcealment = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetErrorRecognition = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxDelay = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetSkipTop = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetSkipBottom = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxRefs = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetSliceFlags = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetSliceCount = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetFieldOrder = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxColorTrc = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxChromaSampleLocation = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxFrameNumber = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxBlockAlign = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetRequestSampleFmt = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxAudioServiceType = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxHasBFrames = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncAVCodecCtxHasBFrames.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecCtxId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() > 0);
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodeccontext_active_thread_type");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecCtxActiveThreadType = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetThreadType = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxSetThreadCount = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxThreadCount = FuncInst->getHostFunc();

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

  auto &HostFuncAVCodecCtxColorPrimaries = FuncInst->getHostFunc();

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
