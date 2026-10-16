// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVDeviceFreeListDevicesNullSafeAndIdempotent) {
  ASSERT_TRUE(AVDeviceMod != nullptr);
  ASSERT_TRUE(AVUtilMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  EXPECT_EQ(Run(AVDeviceMod.get(),
                "wasmedge_ffmpeg_avdevice_avdevice_free_list_devices",
                {UINT32_C(0)}),
            static_cast<int32_t>(ErrNo::Success));
  EXPECT_EQ(Run(AVDeviceMod.get(),
                "wasmedge_ffmpeg_avdevice_avdevice_free_list_devices",
                {UINT32_C(999999)}),
            static_cast<int32_t>(ErrNo::Success));

  uint32_t FramePtr = UINT32_C(4);
  initEmptyFrame(FramePtr);
  uint32_t FrameId = readUInt32(MemInst, FramePtr);
  ASSERT_TRUE(FrameId > 0);
  EXPECT_EQ(Run(AVDeviceMod.get(),
                "wasmedge_ffmpeg_avdevice_avdevice_free_list_devices",
                {FrameId}),
            static_cast<int32_t>(ErrNo::InternalError));

  EXPECT_EQ(
      Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
      static_cast<int32_t>(ErrNo::Success));
}

TEST_F(FFmpegTest, AVDeviceListDevicesFailureStoresNothing) {
  ASSERT_TRUE(AVDeviceMod != nullptr);
  ASSERT_TRUE(AVFormatMod != nullptr);

  auto Run = [&](WasmEdge::Runtime::Instance::ModuleInstance *TargetMod,
                 const char *Name,
                 std::initializer_list<WasmEdge::ValVariant> Args) {
    auto *Inst = TargetMod->findFuncExports(Name);
    EXPECT_NE(Inst, nullptr);
    EXPECT_TRUE(Inst->getHostFunc().run(CallFrame, Args, Result));
    return Result[0].get<int32_t>();
  };

  uint32_t FormatCtxPtr = UINT32_C(4);
  uint32_t ListPtr = UINT32_C(8);
  uint32_t FilePtr = UINT32_C(100);
  initFormatCtx(FormatCtxPtr, FilePtr,
                std::string("ffmpeg-assets/sample_video.mp4"));
  uint32_t FormatCtxId = readUInt32(MemInst, FormatCtxPtr);
  ASSERT_TRUE(FormatCtxId > 0);

  writeUInt32(MemInst, UINT32_C(0xABCD), ListPtr);
  EXPECT_LT(Run(AVDeviceMod.get(),
                "wasmedge_ffmpeg_avdevice_avdevice_list_devices",
                {FormatCtxId, ListPtr}),
            0);
  EXPECT_EQ(readUInt32(MemInst, ListPtr), UINT32_C(0xABCD));
  EXPECT_EQ(Run(AVDeviceMod.get(),
                "wasmedge_ffmpeg_avdevice_avdevice_nb_devices",
                {UINT32_C(0xABCD)}),
            0);

  EXPECT_EQ(Run(AVFormatMod.get(),
                "wasmedge_ffmpeg_avformat_avformat_close_input", {FormatCtxId}),
            static_cast<int32_t>(ErrNo::Success));
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
