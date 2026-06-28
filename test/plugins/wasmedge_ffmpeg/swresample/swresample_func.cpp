// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSWResampleVersion = FuncInst->getHostFunc();

  {
    EXPECT_TRUE(HostFuncSWResampleVersion.run(CallFrame, {}, Result));
    ASSERT_TRUE((Result[0].get<int32_t>() >> 16) > 0);
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_alloc_set_opts");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrAllocSetOpts = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrFree = FuncInst->getHostFunc();

  FuncInst =
      SWResampleMod->findFuncExports("wasmedge_ffmpeg_swresample_swr_init");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrInit = FuncInst->getHostFunc();

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrInit.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwrId}, Result));
    ASSERT_TRUE(Result[0].get<int32_t>() >= 0);
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_av_opt_set_dict");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVOptSetDict = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrConvertFrame = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrGetDelay = FuncInst->getHostFunc();

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrGetDelay.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwrId, 1},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swresample_configuration_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrConfigLength = FuncInst->getHostFunc();

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
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrConfig = FuncInst->getHostFunc();

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrConfig.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_NE(std::string_view(MemInst->getPointer<char *>(StrPtr),
                               static_cast<size_t>(Length))
                  .find("--"),
              std::string_view::npos);
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swresample_license_length");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrLicenseLen = FuncInst->getHostFunc();

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrLicenseLen.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result));

    Length = Result[0].get<int32_t>();
    ASSERT_TRUE(Length > 0);
  }

  FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swresample_license");
  ASSERT_NE(FuncInst, nullptr);
  ASSERT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncSwrLicense = FuncInst->getHostFunc();

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrLicense.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{StrPtr, Length},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(std::string_view(MemInst->getPointer<char *>(StrPtr),
                               static_cast<size_t>(Length))
                  .find("--"),
              std::string_view::npos);
  }

  // Free once every operation is done: freeing invalidates all ids aliasing the
  // same SwrContext, so an earlier free would strand the reconfigured alias.
  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwrId}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

TEST_F(FFmpegTest, SWRAllocSetOptsFailureInvalidatesAliases) {
  ASSERT_TRUE(SWResampleMod != nullptr);

  auto *FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_alloc_set_opts");
  auto &HostFuncSwrAllocSetOpts = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRAllocSetOpts &>(
      FuncInst->getHostFunc());

  uint32_t SWResamplePtr = UINT32_C(4);
  uint64_t OutChLayoutId = UINT64_C(1) << 1; // Front Right
  uint32_t OutSampleFmtId = 2;               // AV_SAMPLE_FMT_S16
  uint64_t InChLayoutId = UINT64_C(1) << 2;  // Front Center
  uint32_t InSampleFmtId = 3;                // AV_SAMPLE_FMT_S32
  int32_t InSampleRate = 40;
  int32_t LogOffset = 1;

  auto AllocSetOpts = [&](uint32_t ExistingId, int32_t OutSampleRate) {
    HostFuncSwrAllocSetOpts.run(CallFrame,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    SWResamplePtr, ExistingId, OutChLayoutId,
                                    OutSampleFmtId, OutSampleRate, InChLayoutId,
                                    InSampleFmtId, InSampleRate, LogOffset},
                                Result);
    return Result[0].get<int32_t>();
  };

  writeUInt32(MemInst, UINT32_C(0), SWResamplePtr);

  // Allocate a context, then reconfigure it. Reconfiguring mints a second id
  // that aliases the same SwrContext (the established contract).
  ASSERT_EQ(AllocSetOpts(UINT32_C(0), 30),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t FirstId = readUInt32(MemInst, SWResamplePtr);
  ASSERT_TRUE(FirstId > 0);

  ASSERT_EQ(AllocSetOpts(FirstId, 30), static_cast<int32_t>(ErrNo::Success));
  uint32_t SecondId = readUInt32(MemInst, SWResamplePtr);
  ASSERT_TRUE(SecondId > 0);
  ASSERT_NE(SecondId, FirstId);

  auto Env = HostFuncSwrAllocSetOpts.getEnv();
  ASSERT_NE(Env->fetchData(FirstId), nullptr);
  ASSERT_EQ(Env->fetchData(FirstId), Env->fetchData(SecondId));

  // A failing reconfigure (negative out sample rate) makes swr_alloc_set_opts2
  // free the underlying SwrContext. Every id aliasing it -- not just the one
  // passed in -- must be invalidated so a later lookup cannot return a dangling
  // pointer.
  EXPECT_EQ(AllocSetOpts(FirstId, -1),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Env->fetchData(FirstId), nullptr);
  EXPECT_EQ(Env->fetchData(SecondId), nullptr);
}

TEST_F(FFmpegTest, SWRFreeInvalidatesAliases) {
  ASSERT_TRUE(SWResampleMod != nullptr);

  auto *FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_alloc_set_opts");
  auto &HostFuncSwrAllocSetOpts = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRAllocSetOpts &>(
      FuncInst->getHostFunc());

  uint32_t SWResamplePtr = UINT32_C(4);
  uint64_t OutChLayoutId = UINT64_C(1) << 1; // Front Right
  uint32_t OutSampleFmtId = 2;               // AV_SAMPLE_FMT_S16
  uint64_t InChLayoutId = UINT64_C(1) << 2;  // Front Center
  uint32_t InSampleFmtId = 3;                // AV_SAMPLE_FMT_S32
  int32_t InSampleRate = 40;
  int32_t LogOffset = 1;

  auto AllocSetOpts = [&](uint32_t ExistingId, int32_t OutSampleRate) {
    HostFuncSwrAllocSetOpts.run(CallFrame,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    SWResamplePtr, ExistingId, OutChLayoutId,
                                    OutSampleFmtId, OutSampleRate, InChLayoutId,
                                    InSampleFmtId, InSampleRate, LogOffset},
                                Result);
    return Result[0].get<int32_t>();
  };

  writeUInt32(MemInst, UINT32_C(0), SWResamplePtr);

  // Allocate a context, then reconfigure it. Reconfiguring mints a second id
  // that aliases the same SwrContext (the established contract).
  ASSERT_EQ(AllocSetOpts(UINT32_C(0), 30),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t FirstId = readUInt32(MemInst, SWResamplePtr);
  ASSERT_TRUE(FirstId > 0);

  ASSERT_EQ(AllocSetOpts(FirstId, 30), static_cast<int32_t>(ErrNo::Success));
  uint32_t SecondId = readUInt32(MemInst, SWResamplePtr);
  ASSERT_TRUE(SecondId > 0);
  ASSERT_NE(SecondId, FirstId);

  auto Env = HostFuncSwrAllocSetOpts.getEnv();
  ASSERT_NE(Env->fetchData(FirstId), nullptr);
  ASSERT_EQ(Env->fetchData(FirstId), Env->fetchData(SecondId));

  // Freeing via one id frees the underlying SwrContext. Every id aliasing it --
  // not just the one passed in -- must be invalidated so a later lookup cannot
  // return a dangling pointer.
  FuncInst =
      SWResampleMod->findFuncExports("wasmedge_ffmpeg_swresample_swr_free");
  auto &HostFuncSwrFree = FuncInst->getHostFunc();
  HostFuncSwrFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FirstId}, Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Env->fetchData(FirstId), nullptr);
  EXPECT_EQ(Env->fetchData(SecondId), nullptr);
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
