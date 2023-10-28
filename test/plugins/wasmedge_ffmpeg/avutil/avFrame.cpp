#include "avutil/avFrame.h"
#include "../utils.h"
#include "avutil/module.h"

#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

TEST(WasmEdgeAVUtilTest, AVVideoFrame) {

  auto *AVUtilMod = TestUtils::InitModules::createAVUtilModule();
  ASSERT_TRUE(AVUtilMod != nullptr);

  // Create the calling frame with memory instance.
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(5)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  std::array<WasmEdge::ValVariant, 1> Result = {UINT32_C(0)};

  uint32_t AVFramePtr = UINT32_C(8);
  uint32_t AVFrame2Ptr = UINT32_C(40);
  uint32_t DictPtr = UINT32_C(36);

  TestUtils::AVFrame::initVideoFrame(Mod, AVFramePtr, UINT32_C(12),
                                     UINT32_C(16), UINT32_C(20), UINT32_C(24),
                                     UINT32_C(28), Result);
  TestUtils::AVFrame::initVideoFrame(Mod, AVFrame2Ptr, UINT32_C(44),
                                     UINT32_C(48), UINT32_C(52), UINT32_C(56),
                                     UINT32_C(60), Result);

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

    EXPECT_EQ(Result[0].get<int32_t>(), 1920);
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

    EXPECT_EQ(Result[0].get<int32_t>(), 1080);
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

    EXPECT_EQ(Result[0].get<int32_t>(), 1);
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

  uint32_t NumDataPointers = 0;
  {

    HostFuncAVFrameLinesize.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVFrameId, NumDataPointers},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 1920);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_get_buffer");
  auto &HostFuncAVFrameGetBuffer =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameGetBuffer &>(
          FuncInst->getHostFunc());
  {
    // For video, it is 32.
    HostFuncAVFrameGetBuffer.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId, 32},
        Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  //  FuncInst =
  //      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_channels");
  //  auto &HostFuncAVFrameChannels =
  //      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameChannels
  //      &>(
  //          FuncInst->getHostFunc());

  //  FuncInst = AVUtilMod->findFuncExports(
  //      "wasmedge_ffmpeg_avutil_av_frame_set_channels");
  //  auto &HostFuncAVFrameSetChannels = dynamic_cast<
  //      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetChannels &>(
  //      FuncInst->getHostFunc());
  //
  //  {
  //    HostFuncAVFrameSetChannels.run(
  //        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId,
  //        100}, Result);
  //    EXPECT_EQ(Result[0].get<int32_t>(),
  //    static_cast<int32_t>(ErrNo::Success));
  //
  //    //    HostFuncAVFrameChannels.run(
  //    //        CallFrame,
  //    std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
  //    //        Result);
  //    //
  //    //    EXPECT_EQ(Result[0].get<int32_t>(), 100);
  //  }

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

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_coded_picture_number");
  auto &HostAVFrameCodedPictureNumber = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameCodedPictureNumber &>(
      FuncInst->getHostFunc());

  {
    HostAVFrameCodedPictureNumber.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_display_picture_number");
  auto &HostAVFrameDisplayPictureNumber = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameDisplayPictureNumber &>(
      FuncInst->getHostFunc());

  {
    HostAVFrameDisplayPictureNumber.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
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
    HostFuncAVFrameSetWidth.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId, 100},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFrameWidth.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 100);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_set_height");
  auto &HostFuncAVFrameSetHeight =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetHeight &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVFrameSetHeight.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId, 100},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFrameHeight.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 100);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_video_format");
  auto &HostFuncAVFrameSetVideoFormat = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetVideoFormat &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVFrameSetVideoFormat.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId, 10},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFrameVideoFormat.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 10);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_pict_type");
  auto &HostFuncAVFrameSetPictType = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetPictType &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVFrameSetPictType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId, 4},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFramePictType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 4);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_colorspace");
  auto &HostFuncAVFrameSetColorSpace = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetColorSpace &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVFrameSetColorSpace.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId, 4},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFrameColorspace.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 4);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_color_range");
  auto &HostFuncAVFrameSetColorRange = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetColorRange &>(
      FuncInst->getHostFunc());

  {
    HostFuncAVFrameSetColorRange.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId, 1},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostFuncAVFrameColorRange.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 1);
  }

  FuncInst = AVUtilMod->findFuncExports(
      "wasmedge_ffmpeg_avutil_av_frame_set_color_trc");
  auto &HostFuncAVFrameSetColorTransferCharacteristic =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::
                       AVFrameSetColorTransferCharacteristic &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVFrameSetColorTransferCharacteristic.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId, 5},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostAVFrameColorTransferCharacteristic.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 5);
  }

  FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_frame_set_pts");
  auto &HostFuncAVFrameSetPts =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVFrameSetPts &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVFrameSetPts.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVFrameId, 10},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    HostAVFramePts.run(CallFrame,
                       std::initializer_list<WasmEdge::ValVariant>{AVFrameId},
                       Result);
    EXPECT_EQ(Result[0].get<int32_t>(), 10);
  }
}
