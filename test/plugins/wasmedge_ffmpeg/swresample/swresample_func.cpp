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

  {
    SwrId = readUInt32(MemInst, SWResamplePtr);
    EXPECT_TRUE(HostFuncSwrFree.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwrId}, Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}

TEST_F(FFmpegTest, SWRAllocSetOptsFailureInvalidatesTheContextId) {
  ASSERT_TRUE(SWResampleMod != nullptr);

  auto *FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_alloc_set_opts");
  auto &HostFuncSwrAllocSetOpts = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRAllocSetOpts &>(
      FuncInst->getHostFunc());

  uint32_t SWResamplePtr = UINT32_C(4);
  uint64_t OutChLayoutId = UINT64_C(1) << 1;
  uint32_t OutSampleFmtId = 2;
  uint64_t InChLayoutId = UINT64_C(1) << 2;
  uint32_t InSampleFmtId = 3;
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

  ASSERT_EQ(AllocSetOpts(UINT32_C(0), 30),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t FirstId = readUInt32(MemInst, SWResamplePtr);
  ASSERT_TRUE(FirstId > 0);

  auto Env = HostFuncSwrAllocSetOpts.getEnv();
  ASSERT_NE(Env->fetchData(FirstId), nullptr);

  // swr_alloc_set_opts2 frees the context it was given when it fails.
  EXPECT_EQ(AllocSetOpts(FirstId, -1),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Env->fetchData(FirstId), nullptr);
}

TEST_F(FFmpegTest, SWRAllocSetOptsPreservesHandleOnInvalidLayout) {
  ASSERT_TRUE(SWResampleMod != nullptr);

  auto *FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_alloc_set_opts");
  auto &HostFuncSwrAllocSetOpts = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRAllocSetOpts &>(
      FuncInst->getHostFunc());

  uint32_t SWResamplePtr = UINT32_C(4);
  uint64_t OutChLayoutId = UINT64_C(1) << 1;
  uint32_t OutSampleFmtId = 2;
  uint64_t InChLayoutId = UINT64_C(1) << 2;
  uint32_t InSampleFmtId = 3;
  int32_t OutSampleRate = 30;
  int32_t InSampleRate = 40;
  int32_t LogOffset = 1;

  auto AllocSetOpts = [&](uint32_t ExistingId, uint64_t OutLayout,
                          uint64_t InLayout) {
    HostFuncSwrAllocSetOpts.run(CallFrame,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    SWResamplePtr, ExistingId, OutLayout,
                                    OutSampleFmtId, OutSampleRate, InLayout,
                                    InSampleFmtId, InSampleRate, LogOffset},
                                Result);
    return Result[0].get<int32_t>();
  };

  writeUInt32(MemInst, UINT32_C(0), SWResamplePtr);

  ASSERT_EQ(AllocSetOpts(UINT32_C(0), OutChLayoutId, InChLayoutId),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t FirstId = readUInt32(MemInst, SWResamplePtr);
  ASSERT_TRUE(FirstId > 0);

  auto Env = HostFuncSwrAllocSetOpts.getEnv();
  ASSERT_NE(Env->fetchData(FirstId), nullptr);

  EXPECT_EQ(AllocSetOpts(FirstId, UINT64_C(0), InChLayoutId),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(readUInt32(MemInst, SWResamplePtr), FirstId);
  EXPECT_NE(Env->fetchData(FirstId), nullptr);

  EXPECT_EQ(AllocSetOpts(FirstId, OutChLayoutId, UINT64_C(0)),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(readUInt32(MemInst, SWResamplePtr), FirstId);
  EXPECT_NE(Env->fetchData(FirstId), nullptr);

  // A result slot the guest did not seed with the id, as the SDK reads a
  // fresh one, receives the id of the context it still owns.
  writeUInt32(MemInst, UINT32_C(0xABCD), SWResamplePtr);
  EXPECT_EQ(AllocSetOpts(FirstId, UINT64_C(0), InChLayoutId),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(readUInt32(MemInst, SWResamplePtr), FirstId);
  EXPECT_NE(Env->fetchData(FirstId), nullptr);

  EXPECT_EQ(AllocSetOpts(FirstId, OutChLayoutId, InChLayoutId),
            static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, SWRAllocSetOptsZeroesTheSlotWithoutAContext) {
  ASSERT_TRUE(SWResampleMod != nullptr);

  auto *FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_alloc_set_opts");
  ASSERT_NE(FuncInst, nullptr);
  auto &HostFuncSwrAllocSetOpts = FuncInst->getHostFunc();

  uint32_t SWResamplePtr = UINT32_C(4);
  uint64_t OutChLayoutId = UINT64_C(1) << 1;
  uint32_t OutSampleFmtId = 2;
  uint64_t InChLayoutId = UINT64_C(1) << 2;
  uint32_t InSampleFmtId = 3;
  int32_t OutSampleRate = 30;
  int32_t InSampleRate = 40;
  int32_t LogOffset = 1;

  // Without a context to keep, a rejected request leaves the null handle in
  // the slot.
  for (auto [OutLayout, InLayout] : {std::pair{UINT64_C(0), InChLayoutId},
                                     std::pair{OutChLayoutId, UINT64_C(0)}}) {
    writeUInt32(MemInst, UINT32_C(0xABCD), SWResamplePtr);
    HostFuncSwrAllocSetOpts.run(CallFrame,
                                std::initializer_list<WasmEdge::ValVariant>{
                                    SWResamplePtr, UINT32_C(0), OutLayout,
                                    OutSampleFmtId, OutSampleRate, InLayout,
                                    InSampleFmtId, InSampleRate, LogOffset},
                                Result);
    EXPECT_EQ(Result[0].get<int32_t>(),
              static_cast<int32_t>(ErrNo::InternalError));
    EXPECT_EQ(readUInt32(MemInst, SWResamplePtr), UINT32_C(0));
  }
}

TEST_F(FFmpegTest, SWRAllocSetOptsPreservesSlotOnUnresolvedId) {
  ASSERT_TRUE(SWResampleMod != nullptr);

  auto *FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_alloc_set_opts");
  auto &HostFuncSwrAllocSetOpts = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRAllocSetOpts &>(
      FuncInst->getHostFunc());

  uint32_t SWResamplePtr = UINT32_C(4);
  uint64_t OutChLayoutId = UINT64_C(1) << 1;
  uint32_t OutSampleFmtId = 2;
  uint64_t InChLayoutId = UINT64_C(1) << 2;
  uint32_t InSampleFmtId = 3;
  int32_t OutSampleRate = 30;
  int32_t InSampleRate = 40;
  int32_t LogOffset = 1;

  uint32_t UnresolvedId = UINT32_C(0xDEADBEEF);
  writeUInt32(MemInst, UnresolvedId, SWResamplePtr);

  HostFuncSwrAllocSetOpts.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  SWResamplePtr, UnresolvedId, OutChLayoutId,
                                  OutSampleFmtId, OutSampleRate, InChLayoutId,
                                  InSampleFmtId, InSampleRate, LogOffset},
                              Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(readUInt32(MemInst, SWResamplePtr), UnresolvedId);
}

TEST_F(FFmpegTest, SWRAllocSetOptsKeepsTheContextId) {
  ASSERT_TRUE(SWResampleMod != nullptr);

  auto *FuncInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_alloc_set_opts");
  auto &HostFuncSwrAllocSetOpts = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::SWResample::SWRAllocSetOpts &>(
      FuncInst->getHostFunc());

  uint32_t SWResamplePtr = UINT32_C(4);
  uint64_t OutChLayoutId = UINT64_C(1) << 1;
  uint32_t OutSampleFmtId = 2;
  uint64_t InChLayoutId = UINT64_C(1) << 2;
  uint32_t InSampleFmtId = 3;
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

  ASSERT_EQ(AllocSetOpts(UINT32_C(0), 30),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t FirstId = readUInt32(MemInst, SWResamplePtr);
  ASSERT_TRUE(FirstId > 0);

  // Reconfiguring the context hands its own id back; an extra id for the
  // same context could only be released together with it.
  for (int32_t OutSampleRate : {30, 60}) {
    ASSERT_EQ(AllocSetOpts(FirstId, OutSampleRate),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(readUInt32(MemInst, SWResamplePtr), FirstId);
  }
  auto Env = HostFuncSwrAllocSetOpts.getEnv();
  ASSERT_NE(Env->fetchData(FirstId), nullptr);
  EXPECT_EQ(Env->fetchData(FirstId + 1), nullptr);

  FuncInst =
      SWResampleMod->findFuncExports("wasmedge_ffmpeg_swresample_swr_free");
  auto &HostFuncSwrFree = FuncInst->getHostFunc();
  HostFuncSwrFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{FirstId}, Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Env->fetchData(FirstId), nullptr);
}

TEST_F(FFmpegTest, SWRConvertFrameRejectsUnresolvedFrameHandles) {
  ASSERT_TRUE(SWResampleMod != nullptr);

  auto *AllocInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_alloc_set_opts");
  ASSERT_NE(AllocInst, nullptr);
  auto &HostFuncSwrAllocSetOpts = AllocInst->getHostFunc();

  uint32_t SWResamplePtr = UINT32_C(4);
  uint64_t OutChLayoutId = UINT64_C(1) << 1;
  uint32_t OutSampleFmtId = 2;
  uint64_t InChLayoutId = UINT64_C(1) << 2;
  uint32_t InSampleFmtId = 3;
  writeUInt32(MemInst, UINT32_C(0), SWResamplePtr);
  HostFuncSwrAllocSetOpts.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  SWResamplePtr, UINT32_C(0), OutChLayoutId,
                                  OutSampleFmtId, INT32_C(30), InChLayoutId,
                                  InSampleFmtId, INT32_C(40), INT32_C(1)},
                              Result);
  ASSERT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t SwrId = readUInt32(MemInst, SWResamplePtr);
  ASSERT_TRUE(SwrId > 0);

  auto *ConvertInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_convert_frame");
  ASSERT_NE(ConvertInst, nullptr);
  auto &HostFuncConvertFrame = ConvertInst->getHostFunc();

  uint32_t UnresolvedId = UINT32_C(999999);
  HostFuncConvertFrame.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               SwrId, UnresolvedId, UINT32_C(0)},
                           Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));
  HostFuncConvertFrame.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
                               SwrId, UINT32_C(0), UnresolvedId},
                           Result);
  EXPECT_EQ(Result[0].get<int32_t>(),
            static_cast<int32_t>(ErrNo::InternalError));

  auto *FreeInst =
      SWResampleMod->findFuncExports("wasmedge_ffmpeg_swresample_swr_free");
  ASSERT_NE(FreeInst, nullptr);
  auto &HostFuncSwrFree = FreeInst->getHostFunc();
  HostFuncSwrFree.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwrId}, Result);
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, SWRConvertFrameRejectsUnallocatedInput) {
  ASSERT_TRUE(SWResampleMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t SWResamplePtr = UINT32_C(4);
  uint32_t InFramePtr = UINT32_C(8);
  uint32_t OutFramePtr = UINT32_C(12);
  uint64_t const StereoLayoutId = UINT64_C(1) << 27;
  uint32_t const S16FmtId = UINT32_C(2);

  EXPECT_EQ(
      Run(SWResampleMod.get(), "wasmedge_ffmpeg_swresample_swr_alloc_set_opts",
          {SWResamplePtr, UINT32_C(0), StereoLayoutId, S16FmtId, INT32_C(48000),
           StereoLayoutId, S16FmtId, INT32_C(44100), INT32_C(0)}),
      static_cast<int32_t>(ErrNo::Success));
  uint32_t SwrId = readUInt32(MemInst, SWResamplePtr);
  ASSERT_TRUE(SwrId > 0);
  EXPECT_GE(
      Run(SWResampleMod.get(), "wasmedge_ffmpeg_swresample_swr_init", {SwrId}),
      0);

  auto MakeFrame = [&](uint32_t FramePtr, int32_t SampleRate,
                       int32_t NbSamples) {
    initEmptyFrame(FramePtr);
    uint32_t FrameId = readUInt32(MemInst, FramePtr);
    EXPECT_TRUE(FrameId > 0);
    EXPECT_EQ(Run(AVUtilMod.get(),
                  "wasmedge_ffmpeg_avutil_av_frame_set_audio_format",
                  {FrameId, S16FmtId}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(),
                  "wasmedge_ffmpeg_avutil_av_frame_set_channel_layout",
                  {FrameId, StereoLayoutId}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(),
                  "wasmedge_ffmpeg_avutil_av_frame_set_sample_rate",
                  {FrameId, SampleRate}),
              static_cast<int32_t>(ErrNo::Success));
    if (NbSamples > 0) {
      EXPECT_EQ(Run(AVUtilMod.get(),
                    "wasmedge_ffmpeg_avutil_av_frame_set_nb_samples",
                    {FrameId, NbSamples}),
                static_cast<int32_t>(ErrNo::Success));
    }
    return FrameId;
  };
  uint32_t InFrameId = MakeFrame(InFramePtr, INT32_C(44100), INT32_C(1024));
  uint32_t OutFrameId = MakeFrame(OutFramePtr, INT32_C(48000), INT32_C(0));

  EXPECT_EQ(Run(SWResampleMod.get(),
                "wasmedge_ffmpeg_swresample_swr_convert_frame",
                {SwrId, OutFrameId, InFrameId}),
            static_cast<int32_t>(ErrNo::InternalError));

  EXPECT_EQ(
      Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {InFrameId}),
      static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free",
                {OutFrameId}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(
      Run(SWResampleMod.get(), "wasmedge_ffmpeg_swresample_swr_free", {SwrId}),
      static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, SWRConvertFrameRejectsVideoShapedFrames) {
  ASSERT_TRUE(SWResampleMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t SWResamplePtr = UINT32_C(4);
  uint64_t const StereoLayoutId = UINT64_C(1) << 27;
  uint32_t const S16FmtId = UINT32_C(2);

  EXPECT_EQ(
      Run(SWResampleMod.get(), "wasmedge_ffmpeg_swresample_swr_alloc_set_opts",
          {SWResamplePtr, UINT32_C(0), StereoLayoutId, S16FmtId, INT32_C(48000),
           StereoLayoutId, S16FmtId, INT32_C(44100), INT32_C(0)}),
      static_cast<int32_t>(ErrNo::Success));
  uint32_t SwrId = readUInt32(MemInst, SWResamplePtr);
  ASSERT_TRUE(SwrId > 0);
  EXPECT_GE(
      Run(SWResampleMod.get(), "wasmedge_ffmpeg_swresample_swr_init", {SwrId}),
      0);

  auto MakeFrame = [&](uint32_t FramePtr, int32_t SampleRate, int32_t NbSamples,
                       uint32_t Size) {
    initEmptyFrame(FramePtr);
    uint32_t FrameId = readUInt32(MemInst, FramePtr);
    EXPECT_TRUE(FrameId > 0);
    EXPECT_EQ(Run(AVUtilMod.get(),
                  "wasmedge_ffmpeg_avutil_av_frame_set_audio_format",
                  {FrameId, S16FmtId}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(),
                  "wasmedge_ffmpeg_avutil_av_frame_set_channel_layout",
                  {FrameId, StereoLayoutId}),
              static_cast<int32_t>(ErrNo::Success));
    EXPECT_EQ(Run(AVUtilMod.get(),
                  "wasmedge_ffmpeg_avutil_av_frame_set_sample_rate",
                  {FrameId, SampleRate}),
              static_cast<int32_t>(ErrNo::Success));
    if (NbSamples > 0) {
      EXPECT_EQ(Run(AVUtilMod.get(),
                    "wasmedge_ffmpeg_avutil_av_frame_set_nb_samples",
                    {FrameId, NbSamples}),
                static_cast<int32_t>(ErrNo::Success));
      EXPECT_EQ(Run(AVUtilMod.get(),
                    "wasmedge_ffmpeg_avutil_av_frame_get_buffer",
                    {FrameId, INT32_C(0)}),
                0);
    }
    if (Size > 0) {
      EXPECT_EQ(Run(AVUtilMod.get(),
                    "wasmedge_ffmpeg_avutil_av_frame_set_width",
                    {FrameId, Size}),
                static_cast<int32_t>(ErrNo::Success));
      EXPECT_EQ(Run(AVUtilMod.get(),
                    "wasmedge_ffmpeg_avutil_av_frame_set_height",
                    {FrameId, Size}),
                static_cast<int32_t>(ErrNo::Success));
    }
    return FrameId;
  };
  uint32_t InFrameId =
      MakeFrame(UINT32_C(8), INT32_C(44100), INT32_C(1024), UINT32_C(0));
  // swr_convert_frame allocates an empty output frame with av_frame_get_buffer,
  // which takes the video path for a frame with a width and height and then
  // receives about 1100 stereo samples.
  uint32_t VideoShapedOutId =
      MakeFrame(UINT32_C(12), INT32_C(48000), INT32_C(0), UINT32_C(2));
  uint32_t OutFrameId =
      MakeFrame(UINT32_C(16), INT32_C(48000), INT32_C(0), UINT32_C(0));

  EXPECT_EQ(Run(SWResampleMod.get(),
                "wasmedge_ffmpeg_swresample_swr_convert_frame",
                {SwrId, VideoShapedOutId, InFrameId}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run(SWResampleMod.get(),
                "wasmedge_ffmpeg_swresample_swr_convert_frame",
                {SwrId, OutFrameId, InFrameId}),
            0);

  for (uint32_t Id : {InFrameId, VideoShapedOutId, OutFrameId}) {
    EXPECT_EQ(
        Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {Id}),
        static_cast<int32_t>(ErrNo::Success));
  }
  EXPECT_EQ(
      Run(SWResampleMod.get(), "wasmedge_ffmpeg_swresample_swr_free", {SwrId}),
      static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, SWRGetDelayWithoutInputSampleRate) {
  ASSERT_TRUE(SWResampleMod != nullptr);

  auto *AllocInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_alloc_set_opts");
  ASSERT_NE(AllocInst, nullptr);
  auto *DelayInst = SWResampleMod->findFuncExports(
      "wasmedge_ffmpeg_swresample_swr_get_delay");
  ASSERT_NE(DelayInst, nullptr);
  auto *FreeInst =
      SWResampleMod->findFuncExports("wasmedge_ffmpeg_swresample_swr_free");
  ASSERT_NE(FreeInst, nullptr);

  uint32_t SWResamplePtr = UINT32_C(4);
  uint64_t const StereoLayoutId = UINT64_C(1) << 27;
  uint32_t const S16FmtId = UINT32_C(2);

  // swr_alloc_set_opts2 accepts an input sample rate of 0, and swr_get_delay
  // divides by it while the context has no resampler (SIGFPE on x86-64).
  EXPECT_TRUE(AllocInst->getHostFunc().run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{
          SWResamplePtr, UINT32_C(0), StereoLayoutId, S16FmtId, INT32_C(48000),
          StereoLayoutId, S16FmtId, INT32_C(0), INT32_C(0)},
      Result));
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  uint32_t SwrId = readUInt32(MemInst, SWResamplePtr);
  ASSERT_TRUE(SwrId > 0);

  EXPECT_TRUE(DelayInst->getHostFunc().run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{SwrId, INT64_C(1000)},
      Result));
  EXPECT_EQ(Result[0].get<int64_t>(), 0);

  EXPECT_TRUE(FreeInst->getHostFunc().run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{SwrId}, Result));
  EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, SWRInitFreezesContextOptions) {
  ASSERT_TRUE(SWResampleMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t SwrPtr = UINT32_C(4);
  uint32_t AliasPtr = UINT32_C(8);
  uint32_t DictPtr = UINT32_C(12);
  uint32_t KeyPtr = UINT32_C(100);
  uint32_t ValuePtr = UINT32_C(103);
  uint64_t const StereoLayoutId = UINT64_C(1) << 27 | UINT64_C(3);
  uint32_t const S16FmtId = UINT32_C(2);
  uint32_t const U8FmtId = UINT32_C(1);

  EXPECT_EQ(Run(SWResampleMod.get(),
                "wasmedge_ffmpeg_swresample_swr_alloc_set_opts",
                {SwrPtr, UINT32_C(0), StereoLayoutId, S16FmtId, INT32_C(48000),
                 StereoLayoutId, S16FmtId, INT32_C(48000), INT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));
  uint32_t SwrId = readUInt32(MemInst, SwrPtr);
  ASSERT_TRUE(SwrId > 0);
  initDict(DictPtr, KeyPtr, "osf", ValuePtr, "s16");
  uint32_t DictId = readUInt32(MemInst, DictPtr);
  ASSERT_TRUE(DictId > 0);

  // Before swr_init both ways of changing the options are open.
  EXPECT_GE(Run(SWResampleMod.get(),
                "wasmedge_ffmpeg_swresample_av_opt_set_dict", {SwrId, DictId}),
            0);
  EXPECT_EQ(Run(SWResampleMod.get(),
                "wasmedge_ffmpeg_swresample_swr_alloc_set_opts",
                {AliasPtr, SwrId, StereoLayoutId, S16FmtId, INT32_C(48000),
                 StereoLayoutId, S16FmtId, INT32_C(48000), INT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));
  ASSERT_EQ(
      Run(SWResampleMod.get(), "wasmedge_ffmpeg_swresample_swr_init", {SwrId}),
      0);

  // Once initialised, the converter is built for the options it was given;
  // swr_convert_frame checks frames against the current options, so a
  // later change would make it convert into a buffer sized for the old ones.
  EXPECT_EQ(Run(SWResampleMod.get(),
                "wasmedge_ffmpeg_swresample_swr_alloc_set_opts",
                {AliasPtr, SwrId, StereoLayoutId, U8FmtId, INT32_C(48000),
                 StereoLayoutId, S16FmtId, INT32_C(48000), INT32_C(0)}),
            static_cast<int32_t>(ErrNo::InternalError));
  EXPECT_EQ(Run(SWResampleMod.get(),
                "wasmedge_ffmpeg_swresample_av_opt_set_dict", {SwrId, DictId}),
            static_cast<int32_t>(ErrNo::InternalError));

  EXPECT_EQ(
      Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_dict_free", {DictId}),
      static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(
      Run(SWResampleMod.get(), "wasmedge_ffmpeg_swresample_swr_free", {SwrId}),
      static_cast<int32_t>(ErrNo::Success));
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
