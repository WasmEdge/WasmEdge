#include "swscale/swscale_func.h"
#include "swscale/module.h"

#include "../utils.h"
#include <gtest/gtest.h>

using WasmEdge::Host::WasmEdgeFFmpeg::ErrNo;

TEST(WasmEdgeAVSWScaleTest, SWScaleFunc) {

  // Create the wasmedge_process module instance.
  auto *SWScaleMod = TestUtils::InitModules::createSWScaleModule();
  ASSERT_TRUE(SWScaleMod != nullptr);

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

  auto *FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getContext");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetContext =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetContext &>(
          FuncInst->getHostFunc());

  uint32_t SWScalePtr = UINT32_C(1);

  // Allocating SWScale...
  // Filter ID for source and destination is Null.
  {
    //    writeUInt32(MemInst, UINT32_C(0), SWScalePtr);
    EXPECT_TRUE(HostFuncSwsGetContext.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            SWScalePtr, UINT32_C(100), UINT32_C(100), UINT32_C(1),
            UINT32_C(200), UINT32_C(200), UINT32_C(3), UINT32_C(8), UINT32_C(0),
            UINT32_C(0)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SWScalePtr) > 0);
  }

  // Need a way to pass AVFrame with data to test the function.

  // Actual Scale Function...

  //  FuncInst =
  //  SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_scale");
  //  EXPECT_NE(FuncInst, nullptr);
  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //  auto &HostFuncSwsScale =
  //  dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsScale
  //  &>(FuncInst->getHostFunc());
  //
  //  {
  //    EXPECT_TRUE(HostFuncSwsScale.run(CallFrame,
  //        std::initializer_list<WasmEdge::ValVariant>{
  //        SWScalePtr,UINT32_C(100),UINT32_C(100),UINT32_C(1),UINT32_C(200),UINT32_C(200),UINT32_C(3),UINT32_C(8),UINT32_C(0),UINT32_C(0)},
  //        Result));
  //    EXPECT_EQ(Result[0].get<int32_t>(),
  //    static_cast<int32_t>(ErrNo::Success));
  //    ASSERT_TRUE(readUInt32(MemInst,SWScalePtr) > 0);
  //  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_getCachedContext");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetCachedContext = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetCachedContext &>(
      FuncInst->getHostFunc());

  uint32_t SWCachedScalePtr = UINT32_C(4);
  {
    uint32_t SWSScaleID = readUInt32(MemInst, SWScalePtr);
    EXPECT_TRUE(HostFuncSwsGetCachedContext.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            SWCachedScalePtr, SWSScaleID, UINT32_C(100), UINT32_C(100),
            UINT32_C(1), UINT32_C(200), UINT32_C(200), UINT32_C(3), UINT32_C(8),
            UINT32_C(0), UINT32_C(0)},
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
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(3)},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() > 0);

    // AV_PIX_FMT_XVMC is not supported Pixel Format
    EXPECT_TRUE(HostFuncSwsIsSupportedInput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(174)},
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
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(3)},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>() > 0);

    // AV_PIX_FMT_XVMC is not supported Pixel Format
    EXPECT_TRUE(HostFuncSwsIsSupportedOutput.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(174)},
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
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(174)},
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
    uint32_t swscaleId = readUInt32(MemInst, SWScalePtr);
    ASSERT_TRUE(swscaleId > 0);
    EXPECT_TRUE(HostFuncSwsFreeContext.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{swscaleId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    uint32_t swsCachedId = readUInt32(MemInst, SWCachedScalePtr);
    ASSERT_TRUE(readUInt32(MemInst, SWCachedScalePtr) > 0);
    EXPECT_TRUE(HostFuncSwsFreeContext.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{swsCachedId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  {
    uint32_t SWScalePtrInvalid = UINT32_C(80);
    EXPECT_TRUE(HostFuncSwsGetContext.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            SWScalePtrInvalid, UINT32_C(100), UINT32_C(100), UINT32_C(1),
            UINT32_C(-200), UINT32_C(-200), UINT32_C(3), UINT32_C(8),
            UINT32_C(0), UINT32_C(0)},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(),
              static_cast<int32_t>(ErrNo::InternalError));
    ASSERT_TRUE(readUInt32(MemInst, SWScalePtrInvalid) == 0);
  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_getDefaultFilter");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetDefaultFilter = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetDefaultFilter &>(
      FuncInst->getHostFunc());

  uint32_t SwsFilterPtr = UINT32_C(4000);
  {
    EXPECT_TRUE(HostFuncSwsGetDefaultFilter.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{SwsFilterPtr, 10.5, 10.5,
                                                    10.5, 10.5, 10.5, 10.5, 1},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SwsFilterPtr) > 0);
  }

  uint32_t filterId = readUInt32(MemInst, SwsFilterPtr);
  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getLumaH");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetLumaH =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetLumaH &>(
          FuncInst->getHostFunc());

  uint32_t SwsVectorPtr = UINT32_C(4000);
  {
    EXPECT_TRUE(HostFuncSwsGetLumaH.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{filterId, SwsVectorPtr},
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
        std::initializer_list<WasmEdge::ValVariant>{filterId, SwsVectorPtr},
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
        std::initializer_list<WasmEdge::ValVariant>{filterId, SwsVectorPtr},
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
        std::initializer_list<WasmEdge::ValVariant>{filterId, SwsVectorPtr},
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
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{filterId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_allocVec");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsAllocVec =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsAllocVec &>(
          FuncInst->getHostFunc());

  {
    writeUInt32(MemInst, UINT32_C(0), SwsVectorPtr);
    EXPECT_TRUE(HostFuncSwsAllocVec.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{SwsVectorPtr, 20}, Result));
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
    EXPECT_TRUE(HostFuncSwsGetGaussianVec.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{SwsVectorPtr, 20.5, 4.3},
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
    uint32_t swsVecId = readUInt32(MemInst, SwsVectorPtr);
    EXPECT_TRUE(HostFuncSwsScaleVec.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{swsVecId, 20.35},
        Result));
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
    uint32_t swsVecId = readUInt32(MemInst, SwsVectorPtr);
    EXPECT_TRUE(HostFuncSwsNormalizeVec.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{swsVecId, 4.3},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = SWScaleMod->findFuncExports(
      "wasmedge_ffmpeg_swscale_sws_getCoeffVecLength");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsGetCoeffVecLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetCoeffVecLength &>(
      FuncInst->getHostFunc());

  int length;
  {
    uint32_t swsVecId = readUInt32(MemInst, SwsVectorPtr);
    EXPECT_TRUE(HostFuncSwsGetCoeffVecLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{swsVecId},
        Result));
    length = Result[0].get<int32_t>();
    ASSERT_TRUE(length > 0);
  }

  //  // How to pass a Vector to func in this test???
  //  //  FuncInst =
  //  //  SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_getCoeff");
  //  //  EXPECT_NE(FuncInst, nullptr);
  //  //  EXPECT_TRUE(FuncInst->isHostFunction());
  //  //  auto &HostFuncSwsGetCoeff =
  //  //  dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsGetCoeff
  //  //  &>(FuncInst->getHostFunc());
  //  //
  //  //  {
  //  //    std::vector<uint8_t> coeff(length);
  //  //    uint32_t swsVecId = readUInt32(MemInst,SwsVectorPtr);
  //  //    EXPECT_TRUE(HostFuncSwsGetCoeff.run(
  //  //        CallFrame,
  //  //
  //  std::initializer_list<WasmEdge::ValVariant>{swsVecId,&coeff[0],length},
  //  //        Result));
  //  //    EXPECT_EQ(Result[0].get<int32_t>(),
  //  //    static_cast<int32_t>(ErrNo::Success));
  //  //  }
  //  //

  FuncInst = SWScaleMod->findFuncExports("wasmedge_ffmpeg_swscale_sws_freeVec");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwsFreeVec =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWScale::SwsFreeVec &>(
          FuncInst->getHostFunc());

  {
    uint32_t swsVecId = readUInt32(MemInst, SwsVectorPtr);
    EXPECT_TRUE(HostFuncSwsFreeVec.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{swsVecId},
        Result));
  }
}
