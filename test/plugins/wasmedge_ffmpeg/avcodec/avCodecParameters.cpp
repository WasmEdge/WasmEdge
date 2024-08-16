// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avcodec/avCodecParameters.h"
#include "avcodec/module.h"

#include "utils.h"

#include <gtest/gtest.h>

// Testing all AVCodecstruct

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVCodecParameters) {
  ASSERT_TRUE(AVCodecMod != nullptr);

  uint32_t AVCodecParamPtr = UINT32_C(60);

  std::string FileName = "ffmpeg-assets/sample_video.mp4"; // 32 chars
  initFFmpegStructs(UINT32_C(20), UINT32_C(24), UINT32_C(28), FileName,
                    AVCodecParamPtr, UINT32_C(64), UINT32_C(68), UINT32_C(72));

  uint32_t AVCodecParamId = readUInt32(MemInst, AVCodecParamPtr);

  auto *FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodecparam_codec_id");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParamCodecId = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParamCodecId &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecParamCodecId.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecParamId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 27); // H264
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodecparam_codec_type");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParamCodecType = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParamCodecType &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecParamCodecType.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{AVCodecParamId},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), 0); // MediaType Video
  }

  FuncInst = AVCodecMod->findFuncExports(
      "wasmedge_ffmpeg_avcodec_avcodecparam_set_codec_tag");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());

  auto &HostFuncAVCodecParamSetCodecTag = dynamic_cast<
      WasmEdge::Host::WasmEdgeFFmpeg::AVcodec::AVCodecParamSetCodecTag &>(
      FuncInst->getHostFunc());

  {
    EXPECT_TRUE(HostFuncAVCodecParamSetCodecTag.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{AVCodecParamId, 20},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));
  }
}
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
