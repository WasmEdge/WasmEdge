// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameAlloc = FuncInst->getHostFunc();

  uint32_t EmptyFramePtr = UINT32_C(88);

  {
    writeUInt32(MemInst, UINT32_C(0), EmptyFramePtr);
    EXPECT_TRUE(HostFuncAVFrameAlloc.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{EmptyFramePtr},
        Result));

    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_TRUE(readUInt32(MemInst, EmptyFramePtr) > 0);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_free");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameWidth = FuncInst->getHostFunc();

  {
    HostFuncAVFrameWidth.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1920); // Width
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_height");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameHeight = FuncInst->getHostFunc();

  {
    HostFuncAVFrameHeight.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1080); // Height
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_video_format");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameVideoFormat = FuncInst->getHostFunc();

  {
    HostFuncAVFrameVideoFormat.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1); // Video Format
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_isnull");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameIsNull = FuncInst->getHostFunc();

  {
    HostFuncAVFrameIsNull.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_linesize");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
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

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_best_effort_timestamp");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameBestEffortTimestamp = FuncInst->getHostFunc();

  {
    HostFuncAVFrameBestEffortTimestamp.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int64_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_pict_type");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFramePictType = FuncInst->getHostFunc();

  {
    HostFuncAVFramePictType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_interlaced_frame");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameInterlacedFrame = FuncInst->getHostFunc();

  {
    HostFuncAVFrameInterlacedFrame.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_top_field_first");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameTopFieldFirst = FuncInst->getHostFunc();

  {
    HostFuncAVFrameTopFieldFirst.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_palette_has_changed");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFramePaletteHasChanged = FuncInst->getHostFunc();

  {
    HostFuncAVFramePaletteHasChanged.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_colorspace");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameColorspace = FuncInst->getHostFunc();

  {
    HostFuncAVFrameColorspace.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 2);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_color_range");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameColorRange = FuncInst->getHostFunc();

  {
    HostFuncAVFrameColorRange.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_color_trc");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostAVFrameColorTransferCharacteristic = FuncInst->getHostFunc();

  {
    HostAVFrameColorTransferCharacteristic.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 2);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_chroma_location");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostAVFrameChromaLocation = FuncInst->getHostFunc();

  {
    HostAVFrameChromaLocation.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_repeat_pict");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostAVFrameRepeatPict = FuncInst->getHostFunc();

  {
    HostAVFrameRepeatPict.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_flags");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostAVFrameFlags = FuncInst->getHostFunc();

  {
    HostAVFrameFlags.run(CallFrame,
                         std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
                         Result);
    EXPECT_TRUE(Result[0].get<int32_t>() != 1 << 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_quality");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostAVFrameQuality = FuncInst->getHostFunc();

  {
    HostAVFrameQuality.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_metadata");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostAVFrameSetMetadata = FuncInst->getHostFunc();

  {
    HostAVFrameSetMetadata.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, DictId}, Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_key_frame");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostAVFrameKeyFrame = FuncInst->getHostFunc();

  {
    HostAVFrameKeyFrame.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_pts");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostAVFramePts = FuncInst->getHostFunc();

  {
    HostAVFramePts.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
                       Result);
    EXPECT_EQ(Result[0].get<int64_t>(), 0);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_copy");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostAVFrameCopyProps = FuncInst->getHostFunc();

  {
    HostAVFrameCopyProps.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrame2Id, AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int64_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  uint32_t ScratchFramePtr = UINT32_C(92);
  initEmptyFrame(ScratchFramePtr);
  uint32_t ScratchFrameId = readUInt32(MemInst, ScratchFramePtr);
  ASSERT_TRUE(ScratchFrameId > 0);

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_set_width");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameSetWidth = FuncInst->getHostFunc();

  {
    int32_t Width = 100;
    HostFuncAVFrameSetWidth.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ScratchFrameId, Width},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFrameWidth.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ScratchFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), Width);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_set_height");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameSetHeight = FuncInst->getHostFunc();

  int32_t Height = 100;
  {
    HostFuncAVFrameSetHeight.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ScratchFrameId, Height},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFrameHeight.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ScratchFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), Height);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_data");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameSetVideoFormat = FuncInst->getHostFunc();

  {
    uint32_t PixFormatId = 10; // GRAY8
    HostFuncAVFrameSetVideoFormat.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ScratchFrameId,
                                                    PixFormatId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFrameVideoFormat.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ScratchFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), PixFormatId);

    HostFuncAVFrameSetVideoFormat.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), PixFormatId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(),
              static_cast<int32_t>(ErrNo::InternalError));
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_pict_type");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
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

  // Setting the audio fields on the unallocated scratch frame.

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_audio_format");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameSetAudioFormat = FuncInst->getHostFunc();

  uint32_t SampleFormatId = 4;
  {
    HostFuncAVFrameSetAudioFormat.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ScratchFrameId,
                                                    SampleFormatId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_audio_format");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameAudioFormat = FuncInst->getHostFunc();

  {
    HostFuncAVFrameAudioFormat.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ScratchFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), SampleFormatId);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_nb_samples");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameSetNbSamples = FuncInst->getHostFunc();

  int32_t NbSamples = 32;
  {
    HostFuncAVFrameSetNbSamples.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ScratchFrameId, NbSamples},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_nb_samples");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameNbSamples = FuncInst->getHostFunc();

  {
    HostFuncAVFrameNbSamples.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ScratchFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), NbSamples);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_sample_rate");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameSampleRate = FuncInst->getHostFunc();

  {
    HostFuncAVFrameSampleRate.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), SampleRate);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_channels");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameSetChannels = FuncInst->getHostFunc();

  int32_t Channels = 3;
  {
    HostFuncAVFrameSetChannels.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ScratchFrameId, Channels},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_channels");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameChannels = FuncInst->getHostFunc();

  {
    HostFuncAVFrameChannels.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ScratchFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), Channels);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_channel_layout");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameSetChannelLayout = FuncInst->getHostFunc();

  uint64_t ChannelLayout = 1UL << 10;
  {
    HostFuncAVFrameSetChannelLayout.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ScratchFrameId,
                                                    ChannelLayout},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_channel_layout");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameChannelLayout = FuncInst->getHostFunc();

  {
    HostFuncAVFrameChannelLayout.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ScratchFrameId},
        Result);
    EXPECT_EQ(Result[0].get<uint64_t>(), ChannelLayout);
  }

  //  ==========================================================================
  //                            AVFrame Audio Funcs.
  //  ==========================================================================

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_get_buffer");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVFrameGetBuffer = FuncInst->getHostFunc();

  uint32_t BufferFramePtr = UINT32_C(96);
  initEmptyFrame(BufferFramePtr);
  uint32_t BufferFrameId = readUInt32(MemInst, BufferFramePtr);
  ASSERT_TRUE(BufferFrameId > 0);
  {
    HostFuncAVFrameSetWidth.run(CallFrame,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    BufferFrameId, INT32_C(100)},
                                Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    HostFuncAVFrameSetHeight.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{BufferFrameId, Height},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    HostFuncAVFrameSetVideoFormat.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{BufferFrameId,
                                                    UINT32_C(10)},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    // For video, it is 32.
    int32_t Align = 32;
    HostFuncAVFrameGetBuffer.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{BufferFrameId, Align},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  for (uint32_t Id : {ScratchFrameId, BufferFrameId}) {
    HostFuncAVFrameFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{Id}, Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

TEST_F(FFmpegTest, AVFrameDataIndexBounds) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_alloc");
  auto &HostFuncAVFrameAlloc = FuncInst->getHostFunc();

  uint32_t FramePtr = UINT32_C(4);
  HostFuncAVFrameAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FramePtr}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_data");
  auto &HostFuncAVFrameData = FuncInst->getHostFunc();

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
  auto &HostFuncAVFrameAlloc = FuncInst->getHostFunc();

  uint32_t FramePtr = UINT32_C(4);
  HostFuncAVFrameAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FramePtr}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_data");
  auto &HostFuncAVFrameData = FuncInst->getHostFunc();

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

TEST_F(FFmpegTest, AVFrameDataZeroLength) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_alloc");
  auto &HostFuncAVFrameAlloc = FuncInst->getHostFunc();

  uint32_t FramePtr = UINT32_C(4);
  HostFuncAVFrameAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FramePtr}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_data");
  auto &HostFuncAVFrameData = FuncInst->getHostFunc();

  uint32_t BufPtr = UINT32_C(100);
  uint32_t BufLen = UINT32_C(0);
  uint32_t Index = UINT32_C(0);
  HostFuncAVFrameData.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              FrameId, BufPtr, BufLen, Index},
                          Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVFrameDataRejectsOversizedRead) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_alloc");
  auto &HostFuncAVFrameAlloc = FuncInst->getHostFunc();

  uint32_t FramePtr = UINT32_C(4);
  HostFuncAVFrameAlloc.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FramePtr}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_audio_format");
  auto &HostFuncAVFrameSetAudioFormat = FuncInst->getHostFunc();
  uint32_t SampleFormatId = UINT32_C(1);
  HostFuncAVFrameSetAudioFormat.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FrameId, SampleFormatId},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_channel_layout");
  auto &HostFuncAVFrameSetChannelLayout = FuncInst->getHostFunc();
  uint64_t ChannelLayout = 1UL << 10;
  HostFuncAVFrameSetChannelLayout.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FrameId, ChannelLayout},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_nb_samples");
  auto &HostFuncAVFrameSetNbSamples = FuncInst->getHostFunc();
  int32_t NbSamples = 8;
  HostFuncAVFrameSetNbSamples.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FrameId, NbSamples}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_get_buffer");
  auto &HostFuncAVFrameGetBuffer = FuncInst->getHostFunc();
  int32_t Align = 0;
  HostFuncAVFrameGetBuffer.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FrameId, Align},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_data");
  auto &HostFuncAVFrameData = FuncInst->getHostFunc();

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
  auto &HostFuncAVFrameMetadata = FuncInst->getHostFunc();

  uint32_t InvalidFrameId = UINT32_C(0);
  uint32_t DictPtr = UINT32_C(4);
  writeUInt32(MemInst, UINT32_C(0xABCD), DictPtr);
  HostFuncAVFrameMetadata.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{InvalidFrameId, DictPtr},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(readUInt32(MemInst, DictPtr), UINT32_C(0));
}

TEST_F(FFmpegTest, AVFrameSetMetadataMovesOwnedDictAndCopiesViews) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVUtilMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t FrameAPtr = UINT32_C(4);
  uint32_t FrameBPtr = UINT32_C(8);
  uint32_t KeyPtr = UINT32_C(100);
  uint32_t ValuePtr = UINT32_C(103);
  uint32_t OwnedDictPtr = UINT32_C(120);
  uint32_t ViewAPtr = UINT32_C(124);
  uint32_t ViewBPtr = UINT32_C(128);
  uint32_t KeyLenPtr = UINT32_C(132);
  uint32_t ValueLenPtr = UINT32_C(136);
  initEmptyFrame(FrameAPtr);
  initEmptyFrame(FrameBPtr);
  uint32_t FrameAId = readUInt32(MemInst, FrameAPtr);
  uint32_t FrameBId = readUInt32(MemInst, FrameBPtr);
  ASSERT_TRUE(FrameAId > 0);
  ASSERT_TRUE(FrameBId > 0);
  initDict(OwnedDictPtr, KeyPtr, "KEY", ValuePtr, "VALUE");
  uint32_t OwnedDictId = readUInt32(MemInst, OwnedDictPtr);
  ASSERT_TRUE(OwnedDictId > 0);
  auto GetKey = [&](uint32_t DictId) {
    return Run("wasmedge_ffmpeg_avutil_av_dict_get",
               {DictId, KeyPtr, UINT32_C(3), UINT32_C(0), UINT32_C(0),
                KeyLenPtr, ValueLenPtr});
  };

  // An owned dictionary moves into the frame and its handle is consumed.
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_metadata",
                {FrameAId, OwnedDictId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(GetKey(OwnedDictId), static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_dict_free", {OwnedDictId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_metadata", {FrameAId, ViewAPtr}),
      static_cast<int32_t>(ErrNo::Success));
  uint32_t ViewAId = readUInt32(MemInst, ViewAPtr);
  ASSERT_TRUE(ViewAId > 0);
  EXPECT_EQ(GetKey(ViewAId), 1);

  // A borrowed view of another frame's metadata is copied and stays usable.
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_set_metadata", {FrameBId, ViewAId}),
      static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(GetKey(ViewAId), 1);
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_metadata", {FrameBId, ViewBPtr}),
      static_cast<int32_t>(ErrNo::Success));
  uint32_t ViewBId = readUInt32(MemInst, ViewBPtr);
  ASSERT_TRUE(ViewBId > 0);
  EXPECT_EQ(GetKey(ViewBId), 1);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_free", {FrameAId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(GetKey(ViewBId), 1);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_free", {FrameBId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVFrameMetadataLiveView) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_alloc");
  auto &HostFuncAVFrameAlloc = FuncInst->getHostFunc();

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

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_metadata");
  auto &HostFuncAVFrameMetadata = FuncInst->getHostFunc();
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
  auto &HostFuncAVFrameSetMetadata = FuncInst->getHostFunc();
  HostFuncAVFrameSetMetadata.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{FrameId, OwnedDictId},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_get");
  auto &HostFuncAVDictGet = FuncInst->getHostFunc();
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

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_dict_free");
  auto &HostFuncAVDictFree = FuncInst->getHostFunc();
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

TEST_F(FFmpegTest, AVFrameGeometrySettersRejectAllocatedFrame) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVUtilMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t FramePtr = UINT32_C(4);
  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);

  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_set_width", {FrameId, UINT32_C(16)}),
      static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_height",
                {FrameId, UINT32_C(16)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_video_format",
                {FrameId, UINT32_C(1)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_get_buffer", {FrameId, INT32_C(0)}),
      0);

  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_set_width", {FrameId, UINT32_C(32)}),
      static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_height",
                {FrameId, UINT32_C(32)}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_video_format",
                {FrameId, UINT32_C(10)}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_audio_format",
                {FrameId, UINT32_C(2)}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_nb_samples",
                {FrameId, INT32_C(64)}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_channel_layout",
                {FrameId, UINT64_C(3)}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_channels",
                {FrameId, INT32_C(2)}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_width", {FrameId}), 16);

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVFrameGetBufferRejectsReallocationAndBadAlign) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVUtilMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t FramePtr = UINT32_C(4);
  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_set_width", {FrameId, UINT32_C(16)}),
      static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_height",
                {FrameId, UINT32_C(16)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_video_format",
                {FrameId, UINT32_C(1)}),
            static_cast<int32_t>(ErrNo::Success));

  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_get_buffer", {FrameId, INT32_C(-1)}),
      static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_get_buffer",
                {FrameId, INT32_C(0x40000000)}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_get_buffer", {FrameId, INT32_C(0)}),
      0);
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_get_buffer", {FrameId, INT32_C(0)}),
      static_cast<int32_t>(ErrNo::InternalError));

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVFrameGetBufferRejectsMixedGeometry) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVUtilMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t FramePtr = UINT32_C(4);
  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_set_width", {FrameId, UINT32_C(16)}),
      static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_height",
                {FrameId, UINT32_C(16)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_video_format",
                {FrameId, UINT32_C(1)}),
            static_cast<int32_t>(ErrNo::Success));

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_nb_samples",
                {FrameId, INT32_C(4096)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_get_buffer", {FrameId, INT32_C(0)}),
      static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_nb_samples",
                {FrameId, INT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_channel_layout",
                {FrameId, UINT64_C(3)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_get_buffer", {FrameId, INT32_C(0)}),
      static_cast<int32_t>(ErrNo::InternalError));

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_channels",
                {FrameId, INT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_get_buffer", {FrameId, INT32_C(0)}),
      0);

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVFrameSetNbSamplesOnlyShrinksAllocatedFrame) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVUtilMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t FramePtr = UINT32_C(4);
  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_audio_format",
                {FrameId, UINT32_C(2)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_channel_layout",
                {FrameId, UINT64_C(3)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_nb_samples",
                {FrameId, INT32_C(64)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_get_buffer", {FrameId, INT32_C(0)}),
      0);

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_nb_samples",
                {FrameId, INT32_C(32)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_nb_samples",
                {FrameId, INT32_C(48)}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_nb_samples",
                {FrameId, INT32_C(0)}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_nb_samples",
                {FrameId, INT32_C(-1)}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_nb_samples", {FrameId}), 32);

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVFrameCopyRejectsSelf) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVUtilMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t FramePtr = UINT32_C(4);
  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_set_width", {FrameId, UINT32_C(16)}),
      static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_height",
                {FrameId, UINT32_C(16)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_video_format",
                {FrameId, UINT32_C(1)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_get_buffer", {FrameId, INT32_C(0)}),
      0);

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_copy", {FrameId, FrameId}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_copy_props", {FrameId, FrameId}),
      static_cast<int32_t>(ErrNo::InternalError));

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVFrameCopyReportsFailures) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVUtilMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  auto MakeFrame = [&](uint32_t Ptr, uint32_t Size) {
    initEmptyFrame(Ptr);
    uint32_t Id = readUInt32(MemInst, Ptr);
    EXPECT_TRUE(Id > 0);
    EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_width", {Id, Size}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_height", {Id, Size}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_video_format",
                  {Id, UINT32_C(1)}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(
        Run("wasmedge_ffmpeg_avutil_av_frame_get_buffer", {Id, INT32_C(0)}), 0);
    return Id;
  };

  uint32_t SrcId = MakeFrame(UINT32_C(4), UINT32_C(64));
  uint32_t SmallId = MakeFrame(UINT32_C(8), UINT32_C(32));
  uint32_t DestId = MakeFrame(UINT32_C(12), UINT32_C(64));

  // av_frame_copy refuses a destination smaller than the source.
  EXPECT_LT(Run("wasmedge_ffmpeg_avutil_av_frame_copy", {SmallId, SrcId}), 0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_copy", {DestId, SrcId}), 0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_copy_props", {DestId, SrcId}),
            0);

  for (uint32_t Id : {SrcId, SmallId, DestId}) {
    EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_free", {Id}),
              static_cast<int32_t>(ErrNo::Success));
  }
}

TEST_F(FFmpegTest, AVFrameGetBufferFailureLeavesNoResidue) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVUtilMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t FramePtr = UINT32_C(4);
  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_video_format",
                {FrameId, UINT32_C(1)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_set_width", {FrameId, UINT32_C(1)}),
      static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_height",
                {FrameId, UINT32_C(2000000)}),
            static_cast<int32_t>(ErrNo::Success));

  // The plane sizes of this geometry overflow INT_MAX, so av_frame_get_buffer
  // fails after it has already filled in linesize[] for a 1-pixel-wide
  // picture. A later allocation for a wide picture would reuse that
  // linesize and come back far too small.
  int32_t const OverflowRes = Run("wasmedge_ffmpeg_avutil_av_frame_get_buffer",
                                  {FrameId, INT32_C(1024)});
  EXPECT_LT(OverflowRes, 0);
  EXPECT_NE(OverflowRes, static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_linesize", {FrameId, UINT32_C(0)}),
      0);

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_width",
                {FrameId, UINT32_C(100000)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_height",
                {FrameId, UINT32_C(32)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_avutil_av_frame_get_buffer", {FrameId, INT32_C(32)}),
      0);
  EXPECT_GE(
      Run("wasmedge_ffmpeg_avutil_av_frame_linesize", {FrameId, UINT32_C(0)}),
      100000);

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVFrameSetChannelsRebuildsLayout) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = AVUtilMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  auto Layout = [&](uint32_t FrameId) {
    auto *Inst = AVUtilMod->findFuncExports(
        "wasmedge_ffmpeg_avutil_av_frame_channel_layout");
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FrameId},
        Result));
    return Result[0].get<uint64_t>();
  };

  uint32_t FramePtr = UINT32_C(4);
  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);
  uint64_t const StereoId =
      FFmpegUtils::ChannelLayout::intoChannelLayoutID(AV_CH_LAYOUT_STEREO);
  uint64_t const FourPointZeroId =
      FFmpegUtils::ChannelLayout::intoChannelLayoutID(AV_CH_LAYOUT_4POINT0);

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_channel_layout",
                {FrameId, StereoId}),
            static_cast<int32_t>(ErrNo::Success));
  // A count that already matches a valid layout keeps that layout.
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_channels",
                {FrameId, INT32_C(2)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Layout(FrameId), StereoId);
  // Any other count installs the default layout for it, so nb_channels can
  // never disagree with the layout's own order and mask.
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_channels",
                {FrameId, INT32_C(4)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_channels", {FrameId}), 4);
  EXPECT_EQ(Layout(FrameId), FourPointZeroId);
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_set_channels",
                {FrameId, INT32_C(-1)}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_channels", {FrameId}), 4);

  EXPECT_EQ(Run("wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVFrameGetBufferZeroesPlanes) {
  ASSERT_TRUE(AVUtilMod != nullptr);
  ASSERT_TRUE(SWScaleMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  auto MakeFrame = [&](uint32_t Ptr, uint32_t FormatId) {
    initEmptyFrame(Ptr);
    uint32_t Id = readUInt32(MemInst, Ptr);
    EXPECT_TRUE(Id > 0);
    EXPECT_EQ(Run(AVUtilMod.get(),
                  "wasmedge_ffmpeg_avutil_av_frame_set_video_format",
                  {Id, FormatId}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_set_width",
                  {Id, UINT32_C(64)}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_set_height",
                  {Id, UINT32_C(64)}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_get_buffer",
                  {Id, INT32_C(0)}),
              0);
    return Id;
  };
  uint32_t const YUV420PId = UINT32_C(1);
  uint32_t const RGB24Id = UINT32_C(3);
  // A 64x64 YUV420P picture with the default alignment occupies 6272 bytes.
  uint32_t const BufLen = UINT32_C(6272);
  uint32_t BufPtr = UINT32_C(1024);

  // Fill a YUV420P buffer with non-zero samples (black RGB converts to
  // Y=16, U=V=128) and free it, so the allocator has stale data to hand
  // back to the next frame of the same size.
  uint32_t SrcId = MakeFrame(UINT32_C(4), RGB24Id);
  uint32_t DirtyId = MakeFrame(UINT32_C(8), YUV420PId);
  uint32_t SwsPtr = UINT32_C(12);
  EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_getContext",
                {SwsPtr, INT32_C(64), INT32_C(64), RGB24Id, INT32_C(64),
                 INT32_C(64), YUV420PId, INT32_C(2), UINT32_C(0), UINT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t SwsId = readUInt32(MemInst, SwsPtr);
  ASSERT_TRUE(SwsId > 0);
  EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_scale",
                {SwsId, SrcId, INT32_C(0), INT32_C(64), DirtyId}),
            64);
  EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_data",
                {DirtyId, BufPtr, BufLen, UINT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(MemInst->getPointer<uint8_t *>(BufPtr)[0], UINT8_C(16));
  EXPECT_EQ(
      Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_freeContext", {SwsId}),
      static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(
      Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {SrcId}),
      static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(
      Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {DirtyId}),
      static_cast<int32_t>(ErrNo::Success));

  fillMemContent(MemInst, BufPtr, BufLen, UINT8_C(0xAA));
  uint32_t FreshId = MakeFrame(UINT32_C(16), YUV420PId);
  EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_data",
                {FreshId, BufPtr, BufLen, UINT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));
  const uint8_t *Buf = MemInst->getPointer<const uint8_t *>(BufPtr);
  EXPECT_EQ(std::count(Buf, Buf + BufLen, UINT8_C(0)),
            static_cast<std::ptrdiff_t>(BufLen));
  EXPECT_EQ(
      Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {FreshId}),
      static_cast<int32_t>(ErrNo::Success));
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
