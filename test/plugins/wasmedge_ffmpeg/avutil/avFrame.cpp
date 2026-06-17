// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avutil/avFrame.h"
#include "avutil/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVFrame) {
  uint32_t AVFramePtr = UINT32_C(72);
  uint32_t AVFrame2Ptr = UINT32_C(40);
  uint32_t DictPtr = UINT32_C(36);
  uint32_t NumPtr = UINT32_C(80);
  uint32_t DenPtr = UINT32_C(84);
  uint32_t BufPtr = UINT32_C(200); // TO store Frame Data;

  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  initFFmpegStructs(UINT32_C(12), UINT32_C(24), UINT32_C(28), FileName,
                    UINT32_C(60), UINT32_C(64), UINT32_C(68), AVFramePtr);

  initFFmpegStructs(UINT32_C(100), UINT32_C(104), UINT32_C(108), FileName,
                    UINT32_C(112), UINT32_C(116), UINT32_C(120), AVFrame2Ptr);

  uint32_t AVFrameId = readUInt32(MemInst, AVFramePtr);
  uint32_t AVFrame2Id = readUInt32(MemInst, AVFrame2Ptr);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_alloc");
  auto &HostFuncAVFrameAlloc = FuncInst->getHostFunc();

  uint32_t EmptyFramePtr = UINT32_C(64);

  {
    HostFuncAVFrameAlloc.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{EmptyFramePtr},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_TRUE(readUInt32(MemInst, EmptyFramePtr) > 0);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_free");
  auto &HostFuncAVFrameFree = FuncInst->getHostFunc();

  {
    uint32_t EmptyFrameId = readUInt32(MemInst, EmptyFramePtr);
    HostFuncAVFrameFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{EmptyFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_width");
  auto &HostFuncAVFrameWidth = FuncInst->getHostFunc();

  {
    HostFuncAVFrameWidth.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1920); // Width
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_height");
  auto &HostFuncAVFrameHeight = FuncInst->getHostFunc();

  {
    HostFuncAVFrameHeight.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1080); // Height
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_video_format");
  auto &HostFuncAVFrameVideoFormat = FuncInst->getHostFunc();

  {
    HostFuncAVFrameVideoFormat.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1); // Video Format
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_isnull");
  auto &HostFuncAVFrameIsNull = FuncInst->getHostFunc();

  {
    HostFuncAVFrameIsNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_linesize");
  auto &HostFuncAVFrameLinesize = FuncInst->getHostFunc();

  int32_t Stride = 0;
  uint32_t Idx = 0;
  {
    HostFuncAVFrameLinesize.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId, Idx},
        Result);

    Stride = Result[0].get<int32_t>();
    EXPECT_EQ(Stride, 1920);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_get_buffer");
  auto &HostFuncAVFrameGetBuffer = FuncInst->getHostFunc();
  {
    // For video, it is 32.
    int32_t Align = 32;
    HostFuncAVFrameGetBuffer.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, Align}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_best_effort_timestamp");
  auto &HostFuncAVFrameBestEffortTimestamp = FuncInst->getHostFunc();

  {
    HostFuncAVFrameBestEffortTimestamp.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int64_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_pict_type");
  auto &HostFuncAVFramePictType = FuncInst->getHostFunc();

  {
    HostFuncAVFramePictType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_interlaced_frame");
  auto &HostFuncAVFrameInterlacedFrame = FuncInst->getHostFunc();

  {
    HostFuncAVFrameInterlacedFrame.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_top_field_first");
  auto &HostFuncAVFrameTopFieldFirst = FuncInst->getHostFunc();

  {
    HostFuncAVFrameTopFieldFirst.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_palette_has_changed");
  auto &HostFuncAVFramePaletteHasChanged = FuncInst->getHostFunc();

  {
    HostFuncAVFramePaletteHasChanged.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_colorspace");
  auto &HostFuncAVFrameColorspace = FuncInst->getHostFunc();

  {
    HostFuncAVFrameColorspace.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 2);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_color_range");
  auto &HostFuncAVFrameColorRange = FuncInst->getHostFunc();

  {
    HostFuncAVFrameColorRange.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_color_trc");
  auto &HostAVFrameColorTransferCharacteristic = FuncInst->getHostFunc();

  {
    HostAVFrameColorTransferCharacteristic.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 2);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_chroma_location");
  auto &HostAVFrameChromaLocation = FuncInst->getHostFunc();

  {
    HostAVFrameChromaLocation.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_repeat_pict");
  auto &HostAVFrameRepeatPict = FuncInst->getHostFunc();

  {
    HostAVFrameRepeatPict.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_flags");
  auto &HostAVFrameFlags = FuncInst->getHostFunc();

  {
    HostAVFrameFlags.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
                         Result);
    EXPECT_TRUE(Result[0].get<int32_t>() != 1 << 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_quality");
  auto &HostAVFrameQuality = FuncInst->getHostFunc();

  {
    HostAVFrameQuality.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_metadata");
  auto &HostAVFrameMetadata = FuncInst->getHostFunc();

  {
    HostAVFrameMetadata.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, DictPtr},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  uint32_t DictId = readUInt32(MemInst, DictPtr);

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_metadata");
  auto &HostAVFrameSetMetadata = FuncInst->getHostFunc();

  {
    HostAVFrameSetMetadata.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, DictId}, Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_key_frame");
  auto &HostAVFrameKeyFrame = FuncInst->getHostFunc();

  {
    HostAVFrameKeyFrame.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_pts");
  auto &HostAVFramePts = FuncInst->getHostFunc();

  {
    HostAVFramePts.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
                       Result);
    EXPECT_EQ(Result[0].get<int64_t>(), 0);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_copy");
  auto &HostAVFrameCopy = FuncInst->getHostFunc();

  {
    HostAVFrameCopy.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrame2Id, AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int64_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_copy_props");
  auto &HostAVFrameCopyProps = FuncInst->getHostFunc();

  {
    HostAVFrameCopyProps.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrame2Id, AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int64_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_set_width");
  auto &HostFuncAVFrameSetWidth = FuncInst->getHostFunc();

  {
    int32_t Width = 100;
    HostFuncAVFrameSetWidth.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, Width}, Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFrameWidth.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), Width);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_set_height");
  auto &HostFuncAVFrameSetHeight = FuncInst->getHostFunc();

  int32_t Height = 100;
  {
    HostFuncAVFrameSetHeight.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, Height}, Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFrameHeight.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), Height);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_data");
  auto &HostFuncAVFrameData = FuncInst->getHostFunc();

  {
    int32_t Size = 1; // Just reading One byte data for test.
    fillMemContent(MemInst, BufPtr, Size);
    HostFuncAVFrameData.run(CallFrame,
                            std::initializer_list<WasmEdge::ValVariant>{
                                AVFrameId, BufPtr, Size, Idx},
                            Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_video_format");
  auto &HostFuncAVFrameSetVideoFormat = FuncInst->getHostFunc();

  {
    uint32_t PixFormatId = 10; // GRAY8
    HostFuncAVFrameSetVideoFormat.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, PixFormatId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFrameVideoFormat.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), PixFormatId);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_pict_type");
  auto &HostFuncAVFrameSetPictType = FuncInst->getHostFunc();

  {
    int32_t PictureId = 4; // AV_PICTURE_TYPE_S
    HostFuncAVFrameSetPictType.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, PictureId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFramePictType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), PictureId);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_colorspace");
  auto &HostFuncAVFrameSetColorSpace = FuncInst->getHostFunc();

  {
    int32_t ColorSpaceId = 4; // FCC
    HostFuncAVFrameSetColorSpace.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, ColorSpaceId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFrameColorspace.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), ColorSpaceId);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_color_range");
  auto &HostFuncAVFrameSetColorRange = FuncInst->getHostFunc();

  {
    int32_t ColorRangeId = 1; //  MPEG
    HostFuncAVFrameSetColorRange.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, ColorRangeId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFrameColorRange.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), ColorRangeId);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_color_trc");
  auto &HostFuncAVFrameSetColorTransferCharacteristic = FuncInst->getHostFunc();

  {
    int32_t ColorTrcId = 5; // GAMMA28
    HostFuncAVFrameSetColorTransferCharacteristic.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, ColorTrcId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostAVFrameColorTransferCharacteristic.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), ColorTrcId);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_set_pts");
  auto &HostFuncAVFrameSetPts = FuncInst->getHostFunc();

  {
    int64_t Pts = 10;
    HostFuncAVFrameSetPts.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId, Pts},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostAVFramePts.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
                       Result);
    EXPECT_EQ(Result[0].get<int32_t>(), Pts);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_sample_aspect_ratio");
  auto &HostFuncAVFrameSampleAspectRatio = FuncInst->getHostFunc();

  {
    HostFuncAVFrameSampleAspectRatio.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, NumPtr, DenPtr},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  int32_t ColorPrimariesId = 1; // BT709
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_color_primaries");
  auto &HostFuncAVFrameSetColorPrimaries = FuncInst->getHostFunc();

  {
    HostFuncAVFrameSetColorPrimaries.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId,
                                                    ColorPrimariesId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_color_primaries");
  auto &HostFuncAVFrameColorPrimaries = FuncInst->getHostFunc();

  {
    HostFuncAVFrameColorPrimaries.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), ColorPrimariesId);
  }

  //  ==========================================================================
  //                            AVFrame Audio Funcs.
  //  ==========================================================================

  // Setting the fields to Video Frame itself.

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_audio_format");
  auto &HostFuncAVFrameSetAudioFormat = FuncInst->getHostFunc();

  uint32_t SampleFormatId = 4;
  {
    HostFuncAVFrameSetAudioFormat.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, SampleFormatId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_audio_format");
  auto &HostFuncAVFrameAudioFormat = FuncInst->getHostFunc();

  {
    HostFuncAVFrameAudioFormat.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), SampleFormatId);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_nb_samples");
  auto &HostFuncAVFrameSetNbSamples = FuncInst->getHostFunc();

  int32_t NbSamples = 32;
  {
    HostFuncAVFrameSetNbSamples.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, NbSamples},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_nb_samples");
  auto &HostFuncAVFrameNbSamples = FuncInst->getHostFunc();

  {
    HostFuncAVFrameNbSamples.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), NbSamples);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_sample_rate");
  auto &HostFuncAVFrameSetSampleRate = FuncInst->getHostFunc();

  int32_t SampleRate = 10;
  {
    HostFuncAVFrameSetSampleRate.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, SampleRate},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_sample_rate");
  auto &HostFuncAVFrameSampleRate = FuncInst->getHostFunc();

  {
    HostFuncAVFrameSampleRate.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), SampleRate);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_channels");
  auto &HostFuncAVFrameSetChannels = FuncInst->getHostFunc();

  int32_t Channels = 3;
  {
    HostFuncAVFrameSetChannels.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, Channels},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_channels");
  auto &HostFuncAVFrameChannels = FuncInst->getHostFunc();

  {
    HostFuncAVFrameChannels.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), Channels);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_channel_layout");
  auto &HostFuncAVFrameSetChannelLayout = FuncInst->getHostFunc();

  uint64_t ChannelLayout = 1UL << 10;
  {
    HostFuncAVFrameSetChannelLayout.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, ChannelLayout},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_channel_layout");
  auto &HostFuncAVFrameChannelLayout = FuncInst->getHostFunc();

  {
    HostFuncAVFrameChannelLayout.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<uint64_t>(), ChannelLayout);
  }

  //  ==========================================================================
  //                            AVFrame Audio Funcs.
  //  ==========================================================================
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
