// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

// avdevice_free_list_devices is null-safe in FFmpeg: id 0 and an
// already-released id are no-op successes like the other free wrappers; a
// live id of another type is rejected before any id-keyed deallocation.
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

  // The frame handle survived the rejected free and still frees normally.
  EXPECT_EQ(
      Run(AVUtilMod.get(), "wasmedge_ffmpeg_avutil_av_frame_free", {FrameId}),
      static_cast<int32_t>(ErrNo::Success));
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
