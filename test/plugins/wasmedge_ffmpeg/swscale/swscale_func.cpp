// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "swscale/swscale_func.h"
#include "swscale/module.h"

#include "utils.h"

#include <gtest/gtest.h>
#include <limits>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

// ============================================================================
// This test deals with functions related to SwsContext.
// ============================================================================

TEST_F(FFmpegTest, SwsContext) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  auto *FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getContext");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetContext = FuncInst->getHostFunc();

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
  uint32_t PAL8Id = 13;   // PAL8 AVPixFormatId (From Bindings.h)

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsScale = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(
        HostFuncSwsScale.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 SWSScaleId, FrameId, 20, 40, Frame2Id},
                             Result));
    EXPECT_EQ(Result[0].get<int32_t>(),
              static_cast<int32_t>(ErrNo::InternalError));
  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_getCachedContext");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetCachedContext = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsIsSupportedInput = FuncInst->getHostFunc();

  {
    // AV_PIX_FMT_RGB24 is a supported pixel format.
    EXPECT_TRUE(HostFuncSwsIsSupportedInput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{RGB24Id},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() > 0);

    // AV_PIX_FMT_XVMC is not a supported pixel format.
    EXPECT_TRUE(HostFuncSwsIsSupportedInput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{XVMCId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() == 0);

    // AV_PIX_FMT_PAL8 is supported as input but not as output.
    EXPECT_TRUE(HostFuncSwsIsSupportedInput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PAL8Id},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() > 0);
  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_isSupportedOutput");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsIsSupportedOutput = FuncInst->getHostFunc();

  {
    // AV_PIX_FMT_RGB24 is a supported pixel format.
    EXPECT_TRUE(HostFuncSwsIsSupportedOutput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{RGB24Id},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() > 0);

    // AV_PIX_FMT_XVMC is not a supported pixel format.
    EXPECT_TRUE(HostFuncSwsIsSupportedOutput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{XVMCId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() == 0);

    // AV_PIX_FMT_PAL8 is supported as input but not as output.
    EXPECT_TRUE(HostFuncSwsIsSupportedOutput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{PAL8Id},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_isSupportedEndiannessConversion");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsIsSupportedEndiannessConversion = FuncInst->getHostFunc();

  {
    // AV_PIX_FMT_XVMC is not a supported pixel format for
    EXPECT_TRUE(HostFuncSwsIsSupportedEndiannessConversion.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{XVMCId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() == 0);
  }

  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_freeContext");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsFreeContext = FuncInst->getHostFunc();

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
// This test deals with functions related to SwsFilter.
// ============================================================================

TEST_F(FFmpegTest, SwsFilter) {
  ASSERT_TRUE(SWScaleMod != nullptr);
  auto *FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_getDefaultFilter");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetDefaultFilter = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetLumaH = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetLumaV = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetChromaH = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetChromaV = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsFreeFilter = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncSwsFreeFilter.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{FilterId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

// ============================================================================
// This test deals with functions related to SwsVector.
// ============================================================================

TEST_F(FFmpegTest, SwsVector) {
  ASSERT_TRUE(SWScaleMod != nullptr);
  uint32_t SwsVectorPtr = UINT32_C(40);
  uint32_t CoeffPtr = UINT32_C(100);

  auto *FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_allocVec");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsAllocVec = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetGaussianVec = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsScaleVec = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsNormalizeVec = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetCoeffVecLength = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetCoeff = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsFreeVec = FuncInst->getHostFunc();

  {
    uint32_t SwsVecId = readUInt32(MemInst, SwsVectorPtr);
    EXPECT_TRUE(HostFuncSwsFreeVec.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwsVecId},
        Result));
  }
}

// ============================================================================
// This test deals with functions related to Version, Configuration, and
// License.
// ============================================================================

TEST_F(FFmpegTest, SWScaleVersion) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  uint32_t Length = 0;
  uint32_t NamePtr = UINT32_C(8);

  auto *FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_swscale_version");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwscaleVersion = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncSwscaleVersion.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    EXPECT_TRUE((Result[0].get<uint32_t>() >> 16) > 0);
  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_swscale_configuration_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwscaleConfigurationLength = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwscaleConfiguration = FuncInst->getHostFunc();

  {
    HostFuncSwscaleConfiguration.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{NamePtr, Length},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_NE(std::string_view(MemInst->getPointer<char *>(NamePtr),
                               static_cast<size_t>(Length))
                  .find("--"),
              std::string_view::npos);
  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_swscale_license_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwscaleLicenseLength = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwscaleLicense = FuncInst->getHostFunc();

  {
    HostFuncSwscaleLicense.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{NamePtr, Length},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(NamePtr),
                               static_cast<size_t>(Length))
                  .find("--"),
              std::string_view::npos);
  }
}

TEST_F(FFmpegTest, SwsGetCachedContextFailureInvalidatesAliases) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  uint32_t SwsCtxPtr = UINT32_C(4);
  uint32_t SwsCachedCtxPtr = UINT32_C(8);
  uint32_t YUV420PId = 1;
  uint32_t RGB24Id = 3;
  uint32_t SrcWidth = 100;
  uint32_t SrcHeight = 100;
  uint32_t DestWidth = 200;
  uint32_t DestHeight = 200;
  int32_t Flags = 8;
  uint32_t SrcFilterId = 0;
  uint32_t DestFilterId = 0;

  auto *FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getContext");
  auto &HostFuncSwsGetContext =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetContext &>(
          FuncInst->getHostFunc());
  HostFuncSwsGetContext.run(CallFrame,
                            std::initializer_list<WasmEdge::ValVariant>{
                                SwsCtxPtr, SrcWidth, SrcHeight, YUV420PId,
                                DestWidth, DestHeight, RGB24Id, Flags,
                                SrcFilterId, DestFilterId},
                            Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t SwsCtxId = readUInt32(MemInst, SwsCtxPtr);
  ASSERT_TRUE(SwsCtxId > 0);

  auto Env = HostFuncSwsGetContext.getEnv();
  ASSERT_NE(Env->fetchData(SwsCtxId), nullptr);

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_getCachedContext");
  auto &HostFuncSwsGetCachedContext = FuncInst->getHostFunc();

  writeUInt32(MemInst, UINT32_C(0), SwsCachedCtxPtr);
  HostFuncSwsGetCachedContext.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          SwsCachedCtxPtr, SwsCtxId, UINT32_C(0), UINT32_C(0), YUV420PId,
          DestWidth, DestHeight, RGB24Id, Flags, SrcFilterId, DestFilterId},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Env->fetchData(SwsCtxId), nullptr);
}

TEST_F(FFmpegTest, SwsGetCachedContextKeepsTheContextId) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  uint32_t SwsCtxPtr = UINT32_C(4);
  uint32_t SwsCachedCtxPtr = UINT32_C(8);
  uint32_t YUV420PId = 1;
  uint32_t RGB24Id = 3;
  uint32_t SrcWidth = 100;
  uint32_t SrcHeight = 100;
  uint32_t DestWidth = 200;
  uint32_t DestHeight = 200;
  int32_t Flags = 8;
  uint32_t SrcFilterId = 0;
  uint32_t DestFilterId = 0;

  auto *FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getContext");
  auto &HostFuncSwsGetContext =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetContext &>(
          FuncInst->getHostFunc());
  HostFuncSwsGetContext.run(CallFrame,
                            std::initializer_list<WasmEdge::ValVariant>{
                                SwsCtxPtr, SrcWidth, SrcHeight, YUV420PId,
                                DestWidth, DestHeight, RGB24Id, Flags,
                                SrcFilterId, DestFilterId},
                            Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t FirstId = readUInt32(MemInst, SwsCtxPtr);
  ASSERT_TRUE(FirstId > 0);

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_getCachedContext");
  auto &HostFuncSwsGetCachedContext = FuncInst->getHostFunc();
  auto GetCachedContext = [&](uint32_t CtxId, uint32_t DestFormatId) {
    writeUInt32(MemInst, UINT32_C(0), SwsCachedCtxPtr);
    HostFuncSwsGetCachedContext.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            SwsCachedCtxPtr, CtxId, SrcWidth, SrcHeight, YUV420PId, DestWidth,
            DestHeight, DestFormatId, Flags, SrcFilterId, DestFilterId},
        Result);
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    return readUInt32(MemInst, SwsCachedCtxPtr);
  };
  auto Env = HostFuncSwsGetContext.getEnv();

  // An unchanged context keeps its id, as the SDK asks for it every frame;
  // an extra id for it could only be released together with the context.
  for (int I = 0; I < 2; ++I) {
    EXPECT_EQ(GetCachedContext(FirstId, RGB24Id), FirstId);
  }
  ASSERT_NE(Env->fetchData(FirstId), nullptr);
  EXPECT_EQ(Env->fetchData(FirstId + 1), nullptr);

  // A changed request rebuilds the context, which may land at the address
  // of the one it replaced; the old id must not outlive the old context.
  uint32_t SecondId = GetCachedContext(FirstId, YUV420PId);
  ASSERT_TRUE(SecondId > 0);
  ASSERT_NE(Env->fetchData(SecondId), nullptr);
  EXPECT_TRUE(SecondId == FirstId || Env->fetchData(FirstId) == nullptr);

  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_freeContext");
  auto &HostFuncSwsFreeContext = FuncInst->getHostFunc();
  HostFuncSwsFreeContext.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{SecondId}, Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Env->fetchData(SecondId), nullptr);
  EXPECT_EQ(Env->fetchData(FirstId), nullptr);
}

TEST_F(FFmpegTest, SwsGetCoeffBounds) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  uint32_t SwsVectorPtr = UINT32_C(4);
  int32_t VecLength = 5;
  auto *FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_allocVec");
  auto &HostFuncSwsAllocVec = FuncInst->getHostFunc();
  writeUInt32(MemInst, UINT32_C(0), SwsVectorPtr);
  HostFuncSwsAllocVec.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{SwsVectorPtr, VecLength},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t SwsVecId = readUInt32(MemInst, SwsVectorPtr);
  ASSERT_TRUE(SwsVecId > 0);

  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getCoeff");
  auto &HostFuncSwsGetCoeff = FuncInst->getHostFunc();

  uint32_t Available = static_cast<uint32_t>(VecLength) * sizeof(double);
  uint32_t CoeffPtr = UINT32_C(200);
  uint32_t Len = Available + UINT32_C(24);
  fillMemContent(MemInst, CoeffPtr, Len, UINT8_C(0xAA));
  HostFuncSwsGetCoeff.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{SwsVecId, CoeffPtr, Len},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

  char *Buf = MemInst->getPointer<char *>(CoeffPtr);
  for (uint32_t I = 0; I < Len; ++I) {
    EXPECT_EQ(static_cast<uint8_t>(Buf[I]), UINT8_C(0xAA));
  }

  HostFuncSwsGetCoeff.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              SwsVecId, CoeffPtr, Available},
                          Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, SwsCachedContextRejectsUnresolvedAndWrongTypeIds) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  uint32_t CachedPtr = UINT32_C(4);
  uint32_t UnresolvedId = UINT32_C(999999);
  writeUInt32(MemInst, UINT32_C(0), CachedPtr);

  auto *CachedInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_getCachedContext");
  ASSERT_NE(CachedInst, nullptr);
  auto &HostFuncGetCachedContext = CachedInst->getHostFunc();
  HostFuncGetCachedContext.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   CachedPtr, UnresolvedId, UINT32_C(100),
                                   UINT32_C(100), UINT32_C(1), UINT32_C(50),
                                   UINT32_C(50), UINT32_C(1), INT32_C(0),
                                   UINT32_C(0), UINT32_C(0)},
                               Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(readUInt32(MemInst, CachedPtr), UnresolvedId);

  auto *FreeCtxInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_freeContext");
  ASSERT_NE(FreeCtxInst, nullptr);
  auto &HostFuncFreeContext = FreeCtxInst->getHostFunc();
  HostFuncFreeContext.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{UnresolvedId},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

  uint32_t VecPtr = UINT32_C(8);
  auto *AllocVecInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_allocVec");
  ASSERT_NE(AllocVecInst, nullptr);
  auto &HostFuncAllocVec = AllocVecInst->getHostFunc();
  writeUInt32(MemInst, UINT32_C(0), VecPtr);
  HostFuncAllocVec.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{VecPtr, INT32_C(4)}, Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t VecId = readUInt32(MemInst, VecPtr);
  ASSERT_TRUE(VecId > 0);

  HostFuncFreeContext.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{VecId}, Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

  auto *FreeVecInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_freeVec");
  ASSERT_NE(FreeVecInst, nullptr);
  auto &HostFuncFreeVec = FreeVecInst->getHostFunc();
  HostFuncFreeVec.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{VecId}, Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, SwsGetCachedContextRejectionWritesTheContextId) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  uint32_t SwsCtxPtr = UINT32_C(4);
  uint32_t SwsCachedCtxPtr = UINT32_C(8);
  uint32_t YUV420PId = 1;
  uint32_t RGB24Id = 3;
  uint32_t SrcWidth = 100;
  uint32_t SrcHeight = 100;
  uint32_t DestWidth = 200;
  uint32_t DestHeight = 200;
  int32_t Flags = 8;
  uint32_t UnresolvedFilterId = UINT32_C(999999);

  auto *FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getContext");
  auto &HostFuncSwsGetContext =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetContext &>(
          FuncInst->getHostFunc());
  HostFuncSwsGetContext.run(CallFrame,
                            std::initializer_list<WasmEdge::ValVariant>{
                                SwsCtxPtr, SrcWidth, SrcHeight, YUV420PId,
                                DestWidth, DestHeight, RGB24Id, Flags,
                                UINT32_C(0), UINT32_C(0)},
                            Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t SwsCtxId = readUInt32(MemInst, SwsCtxPtr);
  ASSERT_TRUE(SwsCtxId > 0);
  auto Env = HostFuncSwsGetContext.getEnv();

  // The SDK reads a fresh, unseeded result slot after a call whose result it
  // does not check, so a rejected request must leave the id it still owns.
  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_getCachedContext");
  auto &HostFuncSwsGetCachedContext = FuncInst->getHostFunc();
  writeUInt32(MemInst, UINT32_C(0xABCD), SwsCachedCtxPtr);
  HostFuncSwsGetCachedContext.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          SwsCachedCtxPtr, SwsCtxId, SrcWidth, SrcHeight, YUV420PId, DestWidth,
          DestHeight, RGB24Id, Flags, UnresolvedFilterId, UINT32_C(0)},
      Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(readUInt32(MemInst, SwsCachedCtxPtr), SwsCtxId);
  ASSERT_NE(Env->fetchData(SwsCtxId), nullptr);

  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_freeContext");
  auto &HostFuncSwsFreeContext = FuncInst->getHostFunc();
  HostFuncSwsFreeContext.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwsCtxId}, Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Env->fetchData(SwsCtxId), nullptr);
}

TEST_F(FFmpegTest, SwsFailuresZeroTheIdSlot) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = SWScaleMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  uint32_t SlotPtr = UINT32_C(4);
  uint32_t const Unresolved = UINT32_C(0xDEAD);
  uint32_t const YUV420PId = 1;
  uint32_t const RGB24Id = 3;
  auto Fails = [&](const char *Name,
                   std::initializer_list<WasmEdge::ValVariant> Args) {
    writeUInt32(MemInst, UINT32_C(0xABCD), SlotPtr);
    EXPECT_EQ(Run(Name, Args), static_cast<int32_t>(ErrNo::InternalError))
        << Name;
    EXPECT_EQ(readUInt32(MemInst, SlotPtr), UINT32_C(0)) << Name;
  };

  Fails("wasmedge_ffmpeg_swscale_sws_getContext",
        {SlotPtr, UINT32_C(16), UINT32_C(16), YUV420PId, UINT32_C(16),
         UINT32_C(16), RGB24Id, INT32_C(2), Unresolved, UINT32_C(0)});
  Fails("wasmedge_ffmpeg_swscale_sws_getContext",
        {SlotPtr, UINT32_C(0), UINT32_C(0), YUV420PId, UINT32_C(16),
         UINT32_C(16), RGB24Id, INT32_C(2), UINT32_C(0), UINT32_C(0)});
  // Without a context to keep, a rejected cached request clears the slot.
  Fails("wasmedge_ffmpeg_swscale_sws_getCachedContext",
        {SlotPtr, UINT32_C(0), UINT32_C(16), UINT32_C(16), YUV420PId,
         UINT32_C(16), UINT32_C(16), RGB24Id, INT32_C(2), Unresolved,
         UINT32_C(0)});
  for (const char *Getter : {"wasmedge_ffmpeg_swscale_sws_getLumaH",
                             "wasmedge_ffmpeg_swscale_sws_getLumaV",
                             "wasmedge_ffmpeg_swscale_sws_getChromaH",
                             "wasmedge_ffmpeg_swscale_sws_getChromaV"}) {
    Fails(Getter, {Unresolved, SlotPtr});
  }
}

TEST_F(FFmpegTest, SwsAllocVecFailureDoesNotLeakHandle) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  uint32_t SwsVectorPtr = UINT32_C(4);
  auto *FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_allocVec");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsAllocVec =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsAllocVec &>(
          FuncInst->getHostFunc());
  auto Env = HostFuncSwsAllocVec.getEnv();

  for (int I = 0; I < 8; ++I) {
    writeUInt32(MemInst, UINT32_C(0xABCD), SwsVectorPtr);
    EXPECT_TRUE(HostFuncSwsAllocVec.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{SwsVectorPtr, INT32_C(-1)},
        Result));
    EXPECT_EQ(readUInt32(MemInst, SwsVectorPtr), UINT32_C(0));
  }

  writeUInt32(MemInst, UINT32_C(0), SwsVectorPtr);
  EXPECT_TRUE(HostFuncSwsAllocVec.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{SwsVectorPtr, INT32_C(4)},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t VecId = readUInt32(MemInst, SwsVectorPtr);
  ASSERT_TRUE(VecId > 0);
  ASSERT_NE(Env->fetchData(VecId), nullptr);

  auto *FreeVecInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_freeVec");
  ASSERT_NE(FreeVecInst, nullptr);
  auto &HostFuncSwsFreeVec = FreeVecInst->getHostFunc();
  EXPECT_TRUE(HostFuncSwsFreeVec.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{VecId}, Result));
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Env->fetchData(VecId), nullptr);
}

TEST_F(FFmpegTest, SwsFilterOwnsItsVectors) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = SWScaleMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t FilterPtr = UINT32_C(4);
  uint32_t VecPtr = UINT32_C(8);
  float Zero = 0.0f;

  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_getDefaultFilter",
                {FilterPtr, Zero, Zero, Zero, Zero, Zero, Zero, INT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t FilterId = readUInt32(MemInst, FilterPtr);
  ASSERT_TRUE(FilterId > 0);

  // sws_freeFilter frees the filter's own vectors, so freeing one of them
  // releases only the guest's handle and leaves the vector to the filter.
  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_getLumaH", {FilterId, VecPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t DroppedId = readUInt32(MemInst, VecPtr);
  ASSERT_TRUE(DroppedId > 0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_freeVec", {DroppedId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_getCoeffVecLength", {DroppedId}),
            0);

  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_getLumaH", {FilterId, VecPtr}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t VecId = readUInt32(MemInst, VecPtr);
  ASSERT_TRUE(VecId > 0);
  EXPECT_GT(Run("wasmedge_ffmpeg_swscale_sws_getCoeffVecLength", {VecId}), 0);

  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_freeFilter", {FilterId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_getCoeffVecLength", {VecId}), 0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_freeVec", {VecId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, SwsScaleRejectsFramesThatDoNotMatchTheContext) {
  ASSERT_TRUE(SWScaleMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  auto MakeFrame = [&](uint32_t Ptr, uint32_t Size, bool Allocate) {
    initEmptyFrame(Ptr);
    uint32_t Id = readUInt32(MemInst, Ptr);
    EXPECT_TRUE(Id > 0);
    EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_set_width",
                  {Id, Size}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_set_height",
                  {Id, Size}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(),
                  "wasmedge_ffmpeg_avutil_av_frame_set_video_format",
                  {Id, UINT32_C(1)}),
              static_cast<int32_t>(ErrNo::Success));
    if (Allocate) {
      EXPECT_EQ(Run(AVUtilMod.get(),
                    "wasmedge_ffmpeg_avutil_av_frame_get_buffer",
                    {Id, INT32_C(0)}),
                0);
    }
    return Id;
  };

  uint32_t CtxPtr = UINT32_C(4);
  EXPECT_EQ(
      Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_getContext",
          {CtxPtr, UINT32_C(32), UINT32_C(32), UINT32_C(1), UINT32_C(32),
           UINT32_C(32), UINT32_C(1), INT32_C(4), UINT32_C(0), UINT32_C(0)}),
      static_cast<int32_t>(ErrNo::Success));
  uint32_t CtxId = readUInt32(MemInst, CtxPtr);
  ASSERT_TRUE(CtxId > 0);

  uint32_t BigId = MakeFrame(UINT32_C(8), UINT32_C(32), true);
  uint32_t OtherBigId = MakeFrame(UINT32_C(12), UINT32_C(32), true);
  uint32_t SmallId = MakeFrame(UINT32_C(16), UINT32_C(16), true);
  uint32_t EmptyId = MakeFrame(UINT32_C(20), UINT32_C(32), false);

  // Source frame smaller than the context: FFmpeg would read 32 rows of 32
  // pixels out of a 16x16 buffer.
  EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_scale",
                {CtxId, SmallId, INT32_C(0), INT32_C(32), BigId}),
            static_cast<int32_t>(ErrNo::InternalError));
  // Destination frame smaller than the context: 32x32 output into 16x16.
  EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_scale",
                {CtxId, BigId, INT32_C(0), INT32_C(32), SmallId}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_scale",
                {CtxId, EmptyId, INT32_C(0), INT32_C(32), BigId}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_scale",
                {CtxId, BigId, INT32_C(0), INT32_C(32), OtherBigId}),
            32);

  for (uint32_t Id : {BigId, OtherBigId, SmallId, EmptyId}) {
    EXPECT_EQ(
        Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {Id}),
        static_cast<int32_t>(ErrNo::Success));
  }
  EXPECT_EQ(
      Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_freeContext", {CtxId}),
      static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, SwsScaleAcceptsFormatsTheContextRewrites) {
  ASSERT_TRUE(SWScaleMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

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
    EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_set_width",
                  {Id, UINT32_C(32)}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_set_height",
                  {Id, UINT32_C(32)}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(),
                  "wasmedge_ffmpeg_avutil_av_frame_set_video_format",
                  {Id, FormatId}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_get_buffer",
                  {Id, INT32_C(0)}),
              0);
    return Id;
  };

  uint32_t const YUV420PId = 1;
  uint32_t const RGB24Id = 3;
  uint32_t const YUVJ420PId = 14;
  uint32_t const XYZ12LEId = 111;
  uint32_t const RGB0Id = 129;
  struct Case {
    uint32_t Src;
    uint32_t Dst;
  };
  for (const Case &C :
       {Case{YUVJ420PId, RGB24Id}, Case{RGB0Id, RGB24Id},
        Case{XYZ12LEId, RGB24Id}, Case{YUV420PId, YUVJ420PId}}) {
    uint32_t CtxPtr = UINT32_C(4);
    EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_getContext",
                  {CtxPtr, UINT32_C(32), UINT32_C(32), C.Src, UINT32_C(32),
                   UINT32_C(32), C.Dst, INT32_C(4), UINT32_C(0), UINT32_C(0)}),
              static_cast<int32_t>(ErrNo::Success));
    uint32_t CtxId = readUInt32(MemInst, CtxPtr);
    ASSERT_TRUE(CtxId > 0);
    uint32_t SrcId = MakeFrame(UINT32_C(8), C.Src);
    uint32_t DstId = MakeFrame(UINT32_C(12), C.Dst);

    EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_scale",
                  {CtxId, SrcId, INT32_C(0), INT32_C(32), DstId}),
              32)
        << "source format id " << C.Src << ", destination format id " << C.Dst;

    for (uint32_t Id : {SrcId, DstId}) {
      EXPECT_EQ(
          Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {Id}),
          static_cast<int32_t>(ErrNo::Success));
    }
    EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_freeContext",
                  {CtxId}),
              static_cast<int32_t>(ErrNo::Success));
  }
}

TEST_F(FFmpegTest, SwsScaleRejectsSlicesOutsideTheSource) {
  ASSERT_TRUE(SWScaleMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };
  auto MakeFrame = [&](uint32_t Ptr) {
    initEmptyFrame(Ptr);
    uint32_t Id = readUInt32(MemInst, Ptr);
    EXPECT_TRUE(Id > 0);
    EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_set_width",
                  {Id, UINT32_C(32)}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_set_height",
                  {Id, UINT32_C(32)}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(),
                  "wasmedge_ffmpeg_avutil_av_frame_set_video_format",
                  {Id, UINT32_C(1)}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_get_buffer",
                  {Id, INT32_C(0)}),
              0);
    return Id;
  };

  uint32_t CtxPtr = UINT32_C(4);
  EXPECT_EQ(
      Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_getContext",
          {CtxPtr, UINT32_C(32), UINT32_C(32), UINT32_C(1), UINT32_C(32),
           UINT32_C(32), UINT32_C(1), INT32_C(4), UINT32_C(0), UINT32_C(0)}),
      static_cast<int32_t>(ErrNo::Success));
  uint32_t CtxId = readUInt32(MemInst, CtxPtr);
  ASSERT_TRUE(CtxId > 0);
  uint32_t SrcId = MakeFrame(UINT32_C(8));
  uint32_t DstId = MakeFrame(UINT32_C(12));

  // libswscale accepts (-2, 34) on a 32-row source because the slice still
  // ends at row 32, then scales it bottom-up and writes before the
  // destination planes.
  EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_scale",
                {CtxId, SrcId, INT32_C(-2), INT32_C(34), DstId}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_scale",
                {CtxId, SrcId, INT32_C(0), INT32_C(-2), DstId}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_scale",
                {CtxId, SrcId, INT32_C(16), INT32_C(32), DstId}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_scale",
                {CtxId, SrcId, INT32_C(0), INT32_C(32), DstId}),
            32);

  for (uint32_t Id : {SrcId, DstId}) {
    EXPECT_EQ(
        Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {Id}),
        static_cast<int32_t>(ErrNo::Success));
  }
  EXPECT_EQ(
      Run(SWScaleMod.get(), "wasmedge_ffmpeg_swscale_sws_freeContext", {CtxId}),
      static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, SwsAllocVecZeroesCoefficients) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = SWScaleMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t VecPtr = UINT32_C(4);
  uint32_t CoeffPtr = UINT32_C(200);
  int32_t const VecLength = 449;
  uint32_t const Len = static_cast<uint32_t>(VecLength) * sizeof(double);

  // Free a vector of the same size with non-zero coefficients first, so the
  // allocator has stale data to hand back.
  EXPECT_EQ(
      Run("wasmedge_ffmpeg_swscale_sws_getGaussianVec", {VecPtr, 448.0, 1.0}),
      static_cast<int32_t>(ErrNo::Success));
  uint32_t GaussianId = readUInt32(MemInst, VecPtr);
  ASSERT_TRUE(GaussianId > 0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_freeVec", {GaussianId}),
            static_cast<int32_t>(ErrNo::Success));

  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_allocVec", {VecPtr, VecLength}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t VecId = readUInt32(MemInst, VecPtr);
  ASSERT_TRUE(VecId > 0);
  fillMemContent(MemInst, CoeffPtr, Len, UINT8_C(0xAA));
  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_getCoeff", {VecId, CoeffPtr, Len}),
            static_cast<int32_t>(ErrNo::Success));
  uint8_t *Buf = MemInst->getPointer<uint8_t *>(CoeffPtr);
  for (uint32_t I = 0; I < Len; ++I) {
    EXPECT_EQ(Buf[I], UINT8_C(0)) << "byte " << I;
  }

  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_freeVec", {VecId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, SwsGetDefaultFilterRejectsUnboundedParameters) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = SWScaleMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t FilterPtr = UINT32_C(4);
  float Zero = 0.0f;
  float NaN = std::numeric_limits<float>::quiet_NaN();
  auto Rejects = [&](std::initializer_list<WasmEdge::ValVariant> Args) {
    writeUInt32(MemInst, UINT32_C(0xABCD), FilterPtr);
    EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_getDefaultFilter", Args),
              static_cast<int32_t>(ErrNo::InternalError));
    EXPECT_EQ(readUInt32(MemInst, FilterPtr), UINT32_C(0));
  };

  Rejects(
      {FilterPtr, Zero, 1.0f, Zero, Zero, -2147483648.0f, Zero, INT32_C(0)});
  Rejects({FilterPtr, NaN, Zero, Zero, Zero, Zero, Zero, INT32_C(0)});
  Rejects({FilterPtr, 1e9f, Zero, Zero, Zero, Zero, Zero, INT32_C(0)});

  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_getDefaultFilter",
                {FilterPtr, 1.5f, 1.5f, 0.5f, 0.5f, 2.0f, -2.0f, INT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t FilterId = readUInt32(MemInst, FilterPtr);
  ASSERT_TRUE(FilterId > 0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_freeFilter", {FilterId}),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, SwsVectorAllocationsAreBounded) {
  ASSERT_TRUE(SWScaleMod != nullptr);

  auto Run = [&](const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = SWScaleMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t VecPtr = UINT32_C(4);
  auto Rejects = [&](const char *Name,
                     std::initializer_list<WasmEdge::ValVariant> Args) {
    writeUInt32(MemInst, UINT32_C(0xABCD), VecPtr);
    EXPECT_EQ(Run(Name, Args), static_cast<int32_t>(ErrNo::InternalError));
    EXPECT_EQ(readUInt32(MemInst, VecPtr), UINT32_C(0));
  };

  Rejects("wasmedge_ffmpeg_swscale_sws_allocVec", {VecPtr, INT32_C(1 << 20)});
  Rejects("wasmedge_ffmpeg_swscale_sws_getGaussianVec", {VecPtr, 1e6, 1e6});
  Rejects("wasmedge_ffmpeg_swscale_sws_getGaussianVec",
          {VecPtr, std::numeric_limits<double>::quiet_NaN(), 1.0});

  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_allocVec", {VecPtr, INT32_C(8)}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t VecId = readUInt32(MemInst, VecPtr);
  ASSERT_TRUE(VecId > 0);
  EXPECT_EQ(Run("wasmedge_ffmpeg_swscale_sws_freeVec", {VecId}),
            static_cast<int32_t>(ErrNo::Success));
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
