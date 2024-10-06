// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "swscale/swscale_func.h"
#include "swscale/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

// ============================================================================
// This test deals with funcs related to SwsContext
// ============================================================================

TEST_F(FFmpegTest, SwsContext) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  auto *FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getContext");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetContext =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetContext &>(
          FuncInst->getHostFunc());

  uint32_t SWScalePtr = UINT32_C(4);
  uint32_t SWCachedScalePtr = UINT32_C(8);
  uint32_t FramePtr = UINT32_C(72);
  uint32_t Frame2Ptr = UINT32_C(124);

  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  initFFmpegStructs(UINT32_C(12), UINT32_C(24), UINT32_C(28), FileName,
                    UINT32_C(60), UINT32_C(64), UINT32_C(68), FramePtr);

  initEmptyFrame(Frame2Ptr);

  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  uint32_t Frame2Id = readUInt32(MemInst, Frame2Ptr);

  uint32_t YUV420PId = 1; // YUV420P AVPixFormatId (From Bindings.h)
  uint32_t RGB24Id = 3;   // RGB24  AVPixFormatId (From Bindings.h)
  uint32_t XVMCId = 174;  // XVMC AVPixFormatId (From Bindings.h)

  uint32_t SrcWidth = 100;
  uint32_t SrcHeight = 100;
  uint32_t DestWidth = 200;
  uint32_t DestHeight = 200;
  int32_t Flags = 8;
  uint32_t SrcFilterId = 0;
  uint32_t DestFilterId = 0;

  // Allocating SWScale...
  // Filter ID for source and destination is Null.
  {
    EXPECT_TRUE(HostFuncSwsGetContext.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            SWScalePtr, SrcWidth, SrcHeight, YUV420PId, DestWidth, DestHeight,
            RGB24Id, Flags, SrcFilterId, DestFilterId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SWScalePtr) > 0);
  }

  uint32_t SWSScaleId = readUInt32(MemInst, SWScalePtr);
  ASSERT_TRUE(SWSScaleId > 0);

  // Checking correctness of function. Returns Invalid Argument Error.
  FuncInst = SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_scale");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsScale =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsScale &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(
        HostFuncSwsScale.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 SWSScaleId, FrameId, 20, 40, Frame2Id},
                             Result));
    EXPECT_EQ(Result[0].get<int32_t>(), -22);
  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_getCachedContext");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetCachedContext = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetCachedContext &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncSwsGetCachedContext.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            SWCachedScalePtr, SWSScaleId, SrcWidth, SrcHeight, YUV420PId,
            DestWidth, DestHeight, RGB24Id, Flags, SrcFilterId, DestFilterId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SWCachedScalePtr) > 0);
  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_isSupportedInput");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsIsSupportedInput = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsIsSupportedInput &>(
      FuncInst->getHostFunc());

  {
    // AV_PIX_FMT_RGB24 is supported Pixel Format
    EXPECT_TRUE(HostFuncSwsIsSupportedInput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{RGB24Id},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() > 0);

    // AV_PIX_FMT_XVMC is not supported Pixel Format
    EXPECT_TRUE(HostFuncSwsIsSupportedInput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{XVMCId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_isSupportedOutput");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsIsSupportedOutput = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsIsSupportedOutput &>(
      FuncInst->getHostFunc());

  {
    // AV_PIX_FMT_RGB24 is supported Pixel Format
    EXPECT_TRUE(HostFuncSwsIsSupportedOutput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{RGB24Id},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() > 0);

    // AV_PIX_FMT_XVMC is not supported Pixel Format
    EXPECT_TRUE(HostFuncSwsIsSupportedOutput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{XVMCId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_isSupportedEndiannessConversion");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsIsSupportedEndiannessConversion =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::
                       SwsIsSupportedEndiannessConversion &>(
          FuncInst->getHostFunc());

  {
    // AV_PIX_FMT_XVMC is not supported Pixel Format for
    EXPECT_TRUE(HostFuncSwsIsSupportedEndiannessConversion.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{XVMCId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_freeContext");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsFreeContext =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsFreeContext &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncSwsFreeContext.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SWSScaleId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  {
    uint32_t InvalidDestWidth = -200;
    uint32_t InvalidDestHeight = -200;
    uint32_t SWScalePtrInvalid = UINT32_C(80);
    EXPECT_TRUE(HostFuncSwsGetContext.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            SWScalePtrInvalid, SrcWidth, SrcHeight, YUV420PId, InvalidDestWidth,
            InvalidDestHeight, RGB24Id, Flags, SrcFilterId, DestFilterId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(),
              static_cast<int32_t>(ErrNo::InternalError));
    ASSERT_TRUE(readUInt32(MemInst, SWScalePtrInvalid) == 0);
  }
}

// ============================================================================
// This test deals with funcs related to SwsFilter.
// ============================================================================

TEST_F(FFmpegTest, SwsFilter) {
  ASSERT_TRUE(SWScaleMod != nullptr);
  auto *FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_getDefaultFilter");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetDefaultFilter = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetDefaultFilter &>(
      FuncInst->getHostFunc());

  uint32_t SwsFilterPtr = UINT32_C(40);
  {
    float LumaGBlur = 10.5;
    float ChromaGBlur = 10.5;
    float LumaSharpen = 10.5;
    float ChromaSharpen = 10.5;
    float ChromaHShift = 10.5;
    float ChromaVShift = 10.5;
    int32_t Verbose = 1;

    EXPECT_TRUE(HostFuncSwsGetDefaultFilter.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            SwsFilterPtr, LumaGBlur, ChromaGBlur, LumaSharpen, ChromaSharpen,
            ChromaHShift, ChromaVShift, Verbose},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SwsFilterPtr) > 0);
  }

  uint32_t FilterId = readUInt32(MemInst, SwsFilterPtr);
  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getLumaH");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetLumaH =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetLumaH &>(
          FuncInst->getHostFunc());

  uint32_t SwsVectorPtr = UINT32_C(20);
  {
    EXPECT_TRUE(HostFuncSwsGetLumaH.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FilterId, SwsVectorPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SwsVectorPtr) > 0);
  }

  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getLumaV");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetLumaV =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetLumaV &>(
          FuncInst->getHostFunc());

  {
    writeUInt32(MemInst, UINT32_C(0), SwsVectorPtr);
    EXPECT_TRUE(HostFuncSwsGetLumaV.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FilterId, SwsVectorPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SwsVectorPtr) > 0);
  }

  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getChromaH");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetChromaH =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetChromaH &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncSwsGetChromaH.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FilterId, SwsVectorPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SwsVectorPtr) > 0);
  }

  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getChromaV");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetChromaV =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetChromaV &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncSwsGetChromaV.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{FilterId, SwsVectorPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SwsVectorPtr) > 0);
  }

  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_freeFilter");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsFreeFilter =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsFreeFilter &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncSwsFreeFilter.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

// ============================================================================
// This test deals with funcs related to SwsVector.
// ============================================================================

TEST_F(FFmpegTest, SwsVector) {
  ASSERT_TRUE(SWScaleMod != nullptr);
  uint32_t SwsVectorPtr = UINT32_C(40);
  uint32_t CoeffPtr = UINT32_C(100);

  auto *FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_allocVec");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsAllocVec =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsAllocVec &>(
          FuncInst->getHostFunc());

  {
    writeUInt32(MemInst, UINT32_C(0), SwsVectorPtr);
    int32_t Length = 20;
    EXPECT_TRUE(HostFuncSwsAllocVec.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{SwsVectorPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SwsVectorPtr) > 0);
  }

  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getGaussianVec");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetGaussianVec = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetGaussianVec &>(
      FuncInst->getHostFunc());

  {
    writeUInt32(MemInst, UINT32_C(0), SwsVectorPtr);
    double Variance = 20.5;
    double Quality = 4.3;
    EXPECT_TRUE(HostFuncSwsGetGaussianVec.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{SwsVectorPtr, Variance,
                                                    Quality},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SwsVectorPtr) > 0);
  }

  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_scaleVec");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsScaleVec =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsScaleVec &>(
          FuncInst->getHostFunc());

  {
    uint32_t SwsVecId = readUInt32(MemInst, SwsVectorPtr);
    double Scalar = 20.35;
    EXPECT_TRUE(HostFuncSwsScaleVec.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{SwsVecId, Scalar}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_normalizeVec");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsNormalizeVec =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsNormalizeVec &>(
          FuncInst->getHostFunc());

  {
    uint32_t SwsVecId = readUInt32(MemInst, SwsVectorPtr);
    double Height = 4.3;
    EXPECT_TRUE(HostFuncSwsNormalizeVec.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{SwsVecId, Height}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_getCoeffVecLength");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetCoeffVecLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetCoeffVecLength &>(
      FuncInst->getHostFunc());

  int Length = 0;
  {
    uint32_t SwsVecId = readUInt32(MemInst, SwsVectorPtr);
    EXPECT_TRUE(HostFuncSwsGetCoeffVecLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwsVecId},
        Result));
    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getCoeff");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetCoeff =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetCoeff &>(
          FuncInst->getHostFunc());

  fillMemContent(MemInst, CoeffPtr, Length);
  {
    uint32_t SwsVecId = readUInt32(MemInst, SwsVectorPtr);
    EXPECT_TRUE(HostFuncSwsGetCoeff.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{SwsVecId, CoeffPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_freeVec");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsFreeVec =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsFreeVec &>(
          FuncInst->getHostFunc());

  {
    uint32_t SwsVecId = readUInt32(MemInst, SwsVectorPtr);
    EXPECT_TRUE(HostFuncSwsFreeVec.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwsVecId},
        Result));
  }
}

// ============================================================================
// This test deals with funcs related to Version, Configuration and License
// ============================================================================

TEST_F(FFmpegTest, SWScaleVersion) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  uint32_t Length = 0;
  uint32_t NamePtr = UINT32_C(8);

  auto *FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_swscale_version");
  auto &HostFuncSwscaleVersion =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwscaleVersion &>(
          FuncInst->getHostFunc());

  {
    HostFuncSwscaleVersion.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    EXPECT_TRUE(Result[0].get<uint32_t>() > 0);
  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_swscale_configuration_length");
  auto &HostFuncSwscaleConfigurationLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwscaleConfigurationLength &>(
      FuncInst->getHostFunc());

  {
    HostFuncSwscaleConfigurationLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  // Testing Version, Configuration, License
  // Fill NamePtr with 0.
  fillMemContent(MemInst, NamePtr, Length);
  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_swscale_configuration");
  auto &HostFuncSwscaleConfiguration = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwscaleConfiguration &>(
      FuncInst->getHostFunc());

  {
    HostFuncSwscaleConfiguration.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{NamePtr, Length},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_swscale_license_length");
  auto &HostFuncSwscaleLicenseLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwscaleLicenseLength &>(
      FuncInst->getHostFunc());

  {
    HostFuncSwscaleLicenseLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    Length = Result[0].get<int32_t>();
    EXPECT_TRUE(Length > 0);
  }

  // Fill NamePtr with 0.
  fillMemContent(MemInst, NamePtr, Length);
  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_swscale_license");
  auto &HostFuncSwscaleLicense =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwscaleLicense &>(
          FuncInst->getHostFunc());

  {
    HostFuncSwscaleLicense.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{NamePtr, Length},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
