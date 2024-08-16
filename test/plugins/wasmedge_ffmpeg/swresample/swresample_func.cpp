// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "swresample/swresample_func.h"
#include "swresample/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, SWResampleFunc) {
  ASSERT_TRUE(SWResampleMod != nullptr);

  uint32_t DictPtr = UINT32_C(4);
  uint32_t SWResamplePtr = UINT32_C(8);
  uint32_t FramePtr = UINT32_C(72);
  uint32_t Frame2Ptr = UINT32_C(16);
  uint32_t KeyPtr = UINT32_C(100);
  uint32_t ValuePtr = UINT32_C(200);

  initDict(DictPtr, KeyPtr, std::string("Key"), ValuePtr, std::string("Value"));

  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  initFFmpegStructs(UINT32_C(20), UINT32_C(24), UINT32_C(28), FileName,
                    UINT32_C(60), UINT32_C(64), UINT32_C(68), FramePtr);

  uint32_t StrPtr = UINT32_C(76);
  initEmptyFrame(Frame2Ptr);

  uint32_t DictId = readUInt32(MemInst, DictPtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  uint32_t Frame2Id = readUInt32(MemInst, Frame2Ptr);

  auto *FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swresample_version");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSWResampleVersion = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWResampleVersion &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncSWResampleVersion.run(CallFrame, {}, Result));
    ASSERT_TRUE(Result[0].get<int32_t>() > 0);
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_alloc_set_opts");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrAllocSetOpts = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRAllocSetOpts &>(
      FuncInst->getHostFunc());

  // Testing with Null Old SwrCtx. Hence 2nd argument is 0.
  {
    uint32_t SWRCtxId = 0;
    uint64_t OutChLayoutId = 1 << 1; // Front Right
    uint32_t OutSampleFmtId = 2;     // AV_SAMPLE_FMT_S16
    int32_t OutSampleRate = 30;
    uint64_t InChLayoutId = 1 << 2; // FRONT_CENTER
    uint32_t InSampleFmtId = 3;     // AV_SAMPLE_FMT_S32
    int32_t SampleRate = 40;
    int32_t LogOffset = 1;

    EXPECT_TRUE(HostFuncSwrAllocSetOpts.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            SWResamplePtr, SWRCtxId, OutChLayoutId, OutSampleFmtId,
            OutSampleRate, InChLayoutId, InSampleFmtId, SampleRate, LogOffset},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SWResamplePtr) > 0);
  }

  // Test with Existing SwrCtx.
  uint32_t SwrId = readUInt32(MemInst, SWResamplePtr);
  {
    uint32_t SWRCtxId = SwrId;
    uint64_t OutChLayoutId = 1 << 1; // Front Right
    uint32_t OutSampleFmtId = 2;     // AV_SAMPLE_FMT_S16
    int32_t OutSampleRate = 30;
    uint64_t InChLayoutId = 1 << 2; // FRONT_CENTER
    uint32_t InSampleFmtId = 3;     // AV_SAMPLE_FMT_S32
    int32_t SampleRate = 40;
    int32_t LogOffset = 1;
    EXPECT_TRUE(HostFuncSwrAllocSetOpts.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            SWResamplePtr, SWRCtxId, OutChLayoutId, OutSampleFmtId,
            OutSampleRate, InChLayoutId, InSampleFmtId, SampleRate, LogOffset},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    ASSERT_TRUE(readUInt32(MemInst, SWResamplePtr) > 0);
  }

  FuncInst =
      SWResampleMod->findFuncExports("wasmedge_ffmpeg_swresample_swr_free");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrFree =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRFree &>(
          FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncSwrFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwrId}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst =
      SWResampleMod->findFuncExports("wasmedge_ffmpeg_swresample_swr_init");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrInit =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRInit &>(
          FuncInst->getHostFunc());

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrInit.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwrId}, Result));
    ASSERT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_av_opt_set_dict");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVOptSetDict =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWResample::AVOptSetDict &>(
          FuncInst->getHostFunc());

  {
    uint32_t EmptyDictId = 0;
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncAVOptSetDict.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{SwrId, EmptyDictId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncAVOptSetDict.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwrId, DictId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_convert_frame");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrConvertFrame = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRConvertFrame &>(
      FuncInst->getHostFunc());

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrConvertFrame.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{SwrId, Frame2Id, FrameId},
        Result));
    ASSERT_TRUE(Result[0].get<int32_t>());
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_get_delay");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrGetDelay =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRGetDelay &>(
          FuncInst->getHostFunc());

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrGetDelay.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwrId, 1},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swresample_configuration_length");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrConfigLength = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWResampleConfigurationLength
          &>(FuncInst->getHostFunc());

  int32_t Length = 0;
  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrConfigLength.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));
    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swresample_configuration");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrConfig = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWResampleConfiguration &>(
      FuncInst->getHostFunc());

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrConfig.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swresample_license_length");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrLicenseLen = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWResampleLicenseLength &>(
      FuncInst->getHostFunc());

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrLicenseLen.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swresample_license");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrLicense = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWResampleLicense &>(
      FuncInst->getHostFunc());

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrLicense.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
