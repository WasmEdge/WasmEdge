// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avutil/error.h"
#include "avutil/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVError) {
  using namespace std::literals::string_view_literals;
  ASSERT_TRUE(AVUtilMod != nullptr);

  int32_t ErrNum = 35;
  uint32_t ErrStartPtr = UINT32_C(100);
  uint32_t ErrSize = 10;
  fillMemContent(MemInst, ErrStartPtr, ErrSize);
  fillMemContent(MemInst, ErrStartPtr, "Test Error"sv);

  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_strerror");
  auto &HostFuncAVUtilAVStrError =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilAVStrError &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVUtilAVStrError.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(), 0);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_AVERROR");
  auto &HostFuncAVUtilAVError =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilAVError &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVUtilAVError.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ErrNum}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(),
              ErrNum * -1); // Returns Negative, convert to Positive
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_AVUNERROR");
  auto &HostFuncAVUtilAVUNError =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVUtilAVUNError &>(
          FuncInst->getHostFunc());

  {
    HostFuncAVUtilAVUNError.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ErrNum}, Result);

    EXPECT_EQ(Result[0].get<int32_t>(),
              ErrNum * -1); // Returns Negative, convert to Positive
  }
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
