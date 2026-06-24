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
    EXPECT_EQ(Result[0].get<int32_t>(), -22);
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

  // A zero source size makes sws_getCachedContext free the passed context and
  // fail. The id passed in -- now dangling -- must be invalidated so a later
  // lookup cannot return the freed pointer.
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

TEST_F(FFmpegTest, SwsFreeContextInvalidatesAliases) {
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

  // Reconfiguring with identical parameters makes sws_getCachedContext reuse
  // the same SwsContext, so the minted id aliases the same pointer.
  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_getCachedContext");
  auto &HostFuncSwsGetCachedContext = FuncInst->getHostFunc();
  writeUInt32(MemInst, UINT32_C(0), SwsCachedCtxPtr);
  HostFuncSwsGetCachedContext.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          SwsCachedCtxPtr, FirstId, SrcWidth, SrcHeight, YUV420PId, DestWidth,
          DestHeight, RGB24Id, Flags, SrcFilterId, DestFilterId},
      Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t SecondId = readUInt32(MemInst, SwsCachedCtxPtr);
  ASSERT_TRUE(SecondId > 0);
  ASSERT_NE(SecondId, FirstId);

  auto Env = HostFuncSwsGetContext.getEnv();
  ASSERT_NE(Env->fetchData(FirstId), nullptr);
  ASSERT_EQ(Env->fetchData(FirstId), Env->fetchData(SecondId));

  // Freeing via one id frees the underlying SwsContext. Every id aliasing it --
  // not just the one passed in -- must be invalidated so a later lookup cannot
  // return a dangling pointer.
  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_freeContext");
  auto &HostFuncSwsFreeContext = FuncInst->getHostFunc();
  HostFuncSwsFreeContext.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FirstId}, Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Env->fetchData(FirstId), nullptr);
  EXPECT_EQ(Env->fetchData(SecondId), nullptr);
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

  // The vector holds VecLength doubles; the destination is larger and fenced
  // with a sentinel. A guest length beyond the coeff array must be rejected
  // outright, since a partial copy reported as success would let the guest
  // consume the stale tail of its buffer as coefficient data.
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

  // A request bounded by the coefficient array still succeeds.
  HostFuncSwsGetCoeff.run(CallFrame,
                          std::initializer_list<WasmEdge::ValVariant>{
                              SwsVecId, CoeffPtr, Available},
                          Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
