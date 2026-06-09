// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avutil/avFrame.h"
#include "avutil/avDictionary.h"
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
  auto &HostFuncAVFrameAlloc =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameAlloc &>(
          FuncInst->getHostFunc());

  uint32_t EmptyFramePtr = UINT32_C(64);

  {
    HostFuncAVFrameAlloc.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{EmptyFramePtr},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_TRUE(readUInt32(MemInst, EmptyFramePtr) > 0);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_free");
  auto &HostFuncAVFrameFree =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameFree &>(
          FuncInst->getHostFunc());

  {
    uint32_t EmptyFrameId = readUInt32(MemInst, EmptyFramePtr);
    HostFuncAVFrameFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{EmptyFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_width");
  auto &HostFuncAVFrameWidth =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameWidth &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVFrameWidth.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1920); // Width
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_height");
  auto &HostFuncAVFrameHeight =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameHeight &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVFrameHeight.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1080); // Height
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_video_format");
  auto &HostFuncAVFrameVideoFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameVideoFormat &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVFrameVideoFormat.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1); // Video Format
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_isnull");
  auto &HostFuncAVFrameIsNull =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameIsNull &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVFrameIsNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_linesize");
  auto &HostFuncAVFrameLinesize =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameLinesize &>(
          FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameGetBuffer =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameGetBuffer &>(
          FuncInst->getHostFunc());
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
  auto &HostFuncAVFrameBestEffortTimestamp = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameBestEffortTimestamp &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVFrameBestEffortTimestamp.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int64_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_pict_type");
  auto &HostFuncAVFramePictType =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFramePictType &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVFramePictType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_interlaced_frame");
  auto &HostFuncAVFrameInterlacedFrame = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameInterlacedFrame &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVFrameInterlacedFrame.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_top_field_first");
  auto &HostFuncAVFrameTopFieldFirst = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameTopFieldFirst &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVFrameTopFieldFirst.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_palette_has_changed");
  auto &HostFuncAVFramePaletteHasChanged = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFramePaletteHasChanged &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVFramePaletteHasChanged.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_colorspace");
  auto &HostFuncAVFrameColorspace =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameColorSpace &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVFrameColorspace.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 2);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_color_range");
  auto &HostFuncAVFrameColorRange =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameColorRange &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVFrameColorRange.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_color_trc");
  auto &HostAVFrameColorTransferCharacteristic = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameColorTransferCharacteristic
          &>(FuncInst->getHostFunc());

  {
    HostAVFrameColorTransferCharacteristic.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 2);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_chroma_location");
  auto &HostAVFrameChromaLocation = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameChromaLocation &>(
      FuncInst->getHostFunc());

  {
    HostAVFrameChromaLocation.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_repeat_pict");
  auto &HostAVFrameRepeatPict =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameRepeatPict &>(
          FuncInst->getHostFunc());

  {
    HostAVFrameRepeatPict.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_flags");
  auto &HostAVFrameFlags =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameFlags &>(
          FuncInst->getHostFunc());

  {
    HostAVFrameFlags.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
                         Result);
    EXPECT_TRUE(Result[0].get<int32_t>() != 1 << 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_quality");
  auto &HostAVFrameQuality =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameQuality &>(
          FuncInst->getHostFunc());

  {
    HostAVFrameQuality.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_metadata");
  auto &HostAVFrameMetadata =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameMetadata &>(
          FuncInst->getHostFunc());

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
  auto &HostAVFrameSetMetadata = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetMetadata &>(
      FuncInst->getHostFunc());

  {
    HostAVFrameSetMetadata.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, DictId}, Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_key_frame");
  auto &HostAVFrameKeyFrame =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameKeyFrame &>(
          FuncInst->getHostFunc());

  {
    HostAVFrameKeyFrame.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_pts");
  auto &HostAVFramePts =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFramePts &>(
          FuncInst->getHostFunc());

  {
    HostAVFramePts.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
                       Result);
    EXPECT_EQ(Result[0].get<int64_t>(), 0);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_copy");
  auto &HostAVFrameCopy =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameCopy &>(
          FuncInst->getHostFunc());

  {
    HostAVFrameCopy.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrame2Id, AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int64_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_copy_props");
  auto &HostAVFrameCopyProps =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameCopyProps &>(
          FuncInst->getHostFunc());

  {
    HostAVFrameCopyProps.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrame2Id, AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int64_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_set_width");
  auto &HostFuncAVFrameSetWidth =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetWidth &>(
          FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameSetHeight =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetHeight &>(
          FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameData =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameData &>(
          FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameSetVideoFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetVideoFormat &>(
      FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameSetPictType = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetPictType &>(
      FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameSetColorSpace = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetColorSpace &>(
      FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameSetColorRange = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetColorRange &>(
      FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameSetColorTransferCharacteristic =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::
                       AVFrameSetColorTransferCharacteristic &>(
          FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameSetPts =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetPts &>(
          FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameSampleAspectRatio = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSampleAspectRatio &>(
      FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameSetColorPrimaries = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetColorPrimaries &>(
      FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameColorPrimaries = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameColorPrimaries &>(
      FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameSetAudioFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetAudioFormat &>(
      FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameAudioFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameAudioFormat &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVFrameAudioFormat.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), SampleFormatId);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_nb_samples");
  auto &HostFuncAVFrameSetNbSamples = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetNbSamples &>(
      FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameNbSamples =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameNbSamples &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVFrameNbSamples.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), NbSamples);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_sample_rate");
  auto &HostFuncAVFrameSetSampleRate = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetSampleRate &>(
      FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameSampleRate =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSampleRate &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVFrameSampleRate.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), SampleRate);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_channels");
  auto &HostFuncAVFrameSetChannels = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetChannels &>(
      FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameChannels =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameChannels &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVFrameChannels.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), Channels);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_channel_layout");
  auto &HostFuncAVFrameSetChannelLayout = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetChannelLayout &>(
      FuncInst->getHostFunc());

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
  auto &HostFuncAVFrameChannelLayout = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameChannelLayout &>(
      FuncInst->getHostFunc());

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

TEST_F(FFmpegTest, AVFrameDataIndexBounds) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_alloc");
  auto &HostFuncAVFrameAlloc =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameAlloc &>(
          FuncInst->getHostFunc());

  uint32_t FramePtr = UINT32_C(4);
  HostFuncAVFrameAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FramePtr}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_data");
  auto &HostFuncAVFrameData =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameData &>(
          FuncInst->getHostFunc());

  // The destination buffer is valid but the plane index runs far past the
  // fixed-size data[] array; the host must reject it instead of reading a wild
  // pointer from beyond the array.
  uint32_t BufPtr = UINT32_C(100);
  uint32_t BufLen = UINT32_C(4);
  uint32_t OutOfBoundsIndex = UINT32_C(100);
  fillMemContent(MemInst, BufPtr, BufLen);
  HostFuncAVFrameData.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              FrameId, BufPtr, BufLen, OutOfBoundsIndex},
                          Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
}

TEST_F(FFmpegTest, AVFrameDataNullPlane) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_alloc");
  auto &HostFuncAVFrameAlloc =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameAlloc &>(
          FuncInst->getHostFunc());

  uint32_t FramePtr = UINT32_C(4);
  HostFuncAVFrameAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FramePtr}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_data");
  auto &HostFuncAVFrameData =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameData &>(
          FuncInst->getHostFunc());

  // The frame is allocated but never given a buffer, so data[0] is null; the
  // host must reject the read instead of copying from a null plane.
  uint32_t BufPtr = UINT32_C(100);
  uint32_t BufLen = UINT32_C(4);
  uint32_t Index = UINT32_C(0);
  fillMemContent(MemInst, BufPtr, BufLen);
  HostFuncAVFrameData.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              FrameId, BufPtr, BufLen, Index},
                          Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
}

TEST_F(FFmpegTest, AVFrameDataRejectsOversizedRead) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_alloc");
  auto &HostFuncAVFrameAlloc =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameAlloc &>(
          FuncInst->getHostFunc());

  uint32_t FramePtr = UINT32_C(4);
  HostFuncAVFrameAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FramePtr}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);

  // Build a tiny packed-audio frame: 8 samples * 1 channel * U8 gives an
  // 8-byte data[0] plane, far smaller than the destination buffer below.
  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_audio_format");
  auto &HostFuncAVFrameSetAudioFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetAudioFormat &>(
      FuncInst->getHostFunc());
  uint32_t SampleFormatId = UINT32_C(1);
  HostFuncAVFrameSetAudioFormat.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FrameId, SampleFormatId},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_channel_layout");
  auto &HostFuncAVFrameSetChannelLayout = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetChannelLayout &>(
      FuncInst->getHostFunc());
  uint64_t ChannelLayout = 1UL << 10;
  HostFuncAVFrameSetChannelLayout.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FrameId, ChannelLayout},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_nb_samples");
  auto &HostFuncAVFrameSetNbSamples = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetNbSamples &>(
      FuncInst->getHostFunc());
  int32_t NbSamples = 8;
  HostFuncAVFrameSetNbSamples.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FrameId, NbSamples}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_get_buffer");
  auto &HostFuncAVFrameGetBuffer =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameGetBuffer &>(
          FuncInst->getHostFunc());
  int32_t Align = 0;
  HostFuncAVFrameGetBuffer.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FrameId, Align},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_data");
  auto &HostFuncAVFrameData =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameData &>(
          FuncInst->getHostFunc());

  // Ask for far more than the plane backs. The host must reject the request
  // outright: a partial copy reported as success would let the guest consume
  // the stale tail of its buffer as frame data.
  uint32_t BufPtr = UINT32_C(16);
  uint32_t FrameBufLen = UINT32_C(16384);
  uint32_t Index = UINT32_C(0);
  uint8_t const Sentinel = UINT8_C(0xCD);
  fillMemContent(MemInst, BufPtr, FrameBufLen, Sentinel);
  HostFuncAVFrameData.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              FrameId, BufPtr, FrameBufLen, Index},
                          Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

  const uint8_t *Buf = MemInst->getPointer<uint8_t *>(BufPtr);
  EXPECT_EQ(Buf[0], Sentinel);
  EXPECT_EQ(Buf[FrameBufLen / 2], Sentinel);
  EXPECT_EQ(Buf[FrameBufLen - 1], Sentinel);

  // A request within the backed plane still succeeds.
  uint32_t SmallLen = UINT32_C(8);
  HostFuncAVFrameData.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              FrameId, BufPtr, SmallLen, Index},
                          Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVFrameMetadataNullHandle) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_metadata");
  auto &HostFuncAVFrameMetadata =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameMetadata &>(
          FuncInst->getHostFunc());

  // A guest id of 0 resolves to a null AVFrame; the getter must report
  // InternalError instead of dereferencing it.
  uint32_t InvalidFrameId = UINT32_C(0);
  uint32_t DictPtr = UINT32_C(4);
  HostFuncAVFrameMetadata.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{InvalidFrameId, DictPtr},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
}

TEST_F(FFmpegTest, AVFrameMetadataLiveView) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_alloc");
  auto &HostFuncAVFrameAlloc =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameAlloc &>(
          FuncInst->getHostFunc());

  uint32_t FramePtr = UINT32_C(4);
  HostFuncAVFrameAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FramePtr}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);

  uint32_t KeyPtr = UINT32_C(8);
  uint32_t ValuePtr = UINT32_C(11);
  uint32_t OwnedDictPtr = UINT32_C(80);
  initDict(OwnedDictPtr, KeyPtr, "KEY", ValuePtr, "VALUE");
  uint32_t OwnedDictId = readUInt32(MemInst, OwnedDictPtr);
  ASSERT_TRUE(OwnedDictId > 0);

  // Take a metadata handle before any metadata exists.
  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_metadata");
  auto &HostFuncAVFrameMetadata =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameMetadata &>(
          FuncInst->getHostFunc());
  uint32_t MetaDictPtr = UINT32_C(84);
  HostFuncAVFrameMetadata.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FrameId, MetaDictPtr},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t MetaDictId = readUInt32(MemInst, MetaDictPtr);
  ASSERT_TRUE(MetaDictId > 0);

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_metadata");
  auto &HostFuncAVFrameSetMetadata = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetMetadata &>(
      FuncInst->getHostFunc());
  HostFuncAVFrameSetMetadata.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FrameId, OwnedDictId},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  // The handle taken before the set is a live view of the frame's metadata
  // field, so it observes the freshly installed dictionary.
  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_get");
  auto &HostFuncAVDictGet =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictGet &>(
          FuncInst->getHostFunc());
  uint32_t KeyLenPtr = UINT32_C(100);
  uint32_t ValueLenPtr = UINT32_C(104);
  HostFuncAVDictGet.run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            MetaDictId, KeyPtr, UINT32_C(3), UINT32_C(0),
                            UINT32_C(0), KeyLenPtr, ValueLenPtr},
                        Result);
  EXPECT_EQ(Result[0].get<int32_t>(), 1);
  EXPECT_EQ(readUInt32(MemInst, KeyLenPtr), UINT32_C(3));
  EXPECT_EQ(readUInt32(MemInst, ValueLenPtr), UINT32_C(5));

  // Freeing the borrowed handle drops only the id; the frame keeps its
  // metadata and a fresh handle still reads it.
  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_free");
  auto &HostFuncAVDictFree =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDictFree &>(
          FuncInst->getHostFunc());
  HostFuncAVDictFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{MetaDictId},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  HostFuncAVFrameMetadata.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FrameId, MetaDictPtr},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  MetaDictId = readUInt32(MemInst, MetaDictPtr);
  ASSERT_TRUE(MetaDictId > 0);

  // The frame owns an independent copy, so freeing the source dictionary
  // leaves the metadata readable.
  HostFuncAVDictFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{OwnedDictId},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  HostFuncAVDictGet.run(CallFrame,
                        std::initializer_list<WasmEdge::ValVariant>{
                            MetaDictId, KeyPtr, UINT32_C(3), UINT32_C(0),
                            UINT32_C(0), KeyLenPtr, ValueLenPtr},
                        Result);
  EXPECT_EQ(Result[0].get<int32_t>(), 1);
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
