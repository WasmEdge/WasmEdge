// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avutil/avRational.h"
#include "avutil/module.h"

#include "utils.h"

#include <gtest/gtest.h>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

TEST_F(FFmpegTest, AVRational) {
  ASSERT_TRUE(AVUtilMod != nullptr);

  uint32_t NumPtr = UINT32_C(4);
  uint32_t DenPtr = UINT32_C(8);

  // Addition Function
  auto *FuncInst =
      AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_add_q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVAddQ =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVAddQ &>(
          FuncInst->getHostFunc());

  {
    int32_t ANum = 3;
    int32_t ADen = 4;
    int32_t BNum = -6;
    int32_t BDen = 7;
    EXPECT_TRUE(HostFuncAVAddQ.run(CallFrame,
                                   std::initializer_list<WasmEdge::ValVariant>{
                                       ANum, ADen, BNum, BDen, NumPtr, DenPtr},
                                   Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_EQ(readSInt32(MemInst, NumPtr), -3);
    EXPECT_EQ(readSInt32(MemInst, DenPtr), 28);
  }

  // Subtraction Function
  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_sub_q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVSubQ =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVSubQ &>(
          FuncInst->getHostFunc());

  {
    int32_t ANum = -843;
    int32_t ADen = 11;
    int32_t BNum = 38;
    int32_t BDen = 12;

    writeSInt32(MemInst, 0, NumPtr); // Setting value of pointer to 0.
    writeSInt32(MemInst, 0, DenPtr); // Setting value of pointer to 0.
    EXPECT_TRUE(HostFuncAVSubQ.run(CallFrame,
                                   std::initializer_list<WasmEdge::ValVariant>{
                                       ANum, ADen, BNum, BDen, NumPtr, DenPtr},
                                   Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_EQ(readSInt32(MemInst, NumPtr), -5267);
    EXPECT_EQ(readSInt32(MemInst, DenPtr), 66);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_mul_q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVMulQ =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVMulQ &>(
          FuncInst->getHostFunc());

  {
    int32_t ANum = -6;
    int32_t ADen = 7;
    int32_t BNum = 3;
    int32_t BDen = 4;

    writeSInt32(MemInst, 0, NumPtr); // Setting value of pointer to 0.
    writeSInt32(MemInst, 0, DenPtr); // Setting value of pointer to 0.
    EXPECT_TRUE(HostFuncAVMulQ.run(CallFrame,
                                   std::initializer_list<WasmEdge::ValVariant>{
                                       ANum, ADen, BNum, BDen, NumPtr, DenPtr},
                                   Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_EQ(readSInt32(MemInst, NumPtr), -9);
    EXPECT_EQ(readSInt32(MemInst, DenPtr), 14);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_div_q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVDivQ =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVDivQ &>(
          FuncInst->getHostFunc());

  {
    int32_t ANum = -6;
    int32_t ADen = 7;
    int32_t BNum = 3;
    int32_t BDen = 4;

    writeSInt32(MemInst, 0, NumPtr); // Setting value of pointer to 0.
    writeSInt32(MemInst, 0, DenPtr); // Setting value of pointer to 0.
    EXPECT_TRUE(HostFuncAVDivQ.run(CallFrame,
                                   std::initializer_list<WasmEdge::ValVariant>{
                                       ANum, ADen, BNum, BDen, NumPtr, DenPtr},
                                   Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_EQ(readSInt32(MemInst, NumPtr), -8);
    EXPECT_EQ(readSInt32(MemInst, DenPtr), 7);
  }

  // How to Pass a Double functions.

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_d2q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVD2Q =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVD2Q &>(
          FuncInst->getHostFunc());

  {
    double D = 5;
    int32_t Max = 10;

    writeSInt32(MemInst, 0, NumPtr); // Setting value of pointer to 0.
    writeSInt32(MemInst, 0, DenPtr); // Setting value of pointer to 0.

    EXPECT_TRUE(HostFuncAVD2Q.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{D, Max, NumPtr, DenPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<uint32_t>(ErrNo::Success));

    EXPECT_EQ(readSInt32(MemInst, NumPtr), 5);
    EXPECT_EQ(readSInt32(MemInst, DenPtr), 1);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_q2d");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVQ2d =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVQ2d &>(
          FuncInst->getHostFunc());

  {
    // Convert Rational Number to Double.
    int32_t ANum = 1;
    int32_t ADen = 2;

    writeSInt32(MemInst, 0, NumPtr); // Setting value of pointer to 0.
    writeSInt32(MemInst, 0, DenPtr); // Setting value of pointer to 0.
    EXPECT_TRUE(HostFuncAVQ2d.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ANum, ADen},
        Result));
    EXPECT_EQ(Result[0].get<double_t>(), 0.5);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_inv_q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInvQ =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVInvQ &>(
          FuncInst->getHostFunc());

  {
    // Inverse a Rational Number.
    int32_t ANum = -3;
    int32_t ADen = 4;

    writeSInt32(MemInst, 0, NumPtr); // Setting value of pointer to 0.
    writeSInt32(MemInst, 0, DenPtr); // Setting value of pointer to 0.
    EXPECT_TRUE(HostFuncInvQ.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ANum, ADen, NumPtr, DenPtr},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(ErrNo::Success));

    EXPECT_EQ(readSInt32(MemInst, NumPtr), 4);
    EXPECT_EQ(readSInt32(MemInst, DenPtr), -3);
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_q2intfloat");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVQ2IntFloat =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVQ2IntFloat &>(
          FuncInst->getHostFunc());

  {
    int32_t ANum = 1;
    int32_t ADen = 5;

    EXPECT_TRUE(HostFuncAVQ2IntFloat.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{ANum, ADen},
        Result));
    EXPECT_EQ(Result[0].get<uint32_t>(), static_cast<uint32_t>(1045220557));
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_nearer_q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVNearerQ =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVNearerQ &>(
          FuncInst->getHostFunc());

  {
    int32_t ANum = 1;
    int32_t ADen = 3;
    int32_t BNum = 1;
    int32_t BDen = 2;
    int32_t CNum = -1;
    int32_t CDen = 2;

    // B nearer to A
    EXPECT_TRUE(
        HostFuncAVNearerQ.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  ANum, ADen, BNum, BDen, CNum, CDen},
                              Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(1));

    ANum = -1;

    // C nearer to A
    EXPECT_TRUE(
        HostFuncAVNearerQ.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  ANum, ADen, BNum, BDen, CNum, CDen},
                              Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(-1));

    ANum = 0;
    ADen = 0;

    // Both are at same distance
    EXPECT_TRUE(
        HostFuncAVNearerQ.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  ANum, ADen, BNum, BDen, CNum, CDen},
                              Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(0));
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_cmp_q");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVCmpQ =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVCmpQ &>(
          FuncInst->getHostFunc());

  {
    int32_t ANum = 1;
    int32_t ADen = 2;
    int32_t BNum = 2;
    int32_t BDen = 1;
    // A < B
    EXPECT_TRUE(HostFuncAVCmpQ.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ANum, ADen, BNum, BDen},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(-1));

    ANum = 2;
    ADen = 1;
    BNum = 1;
    BDen = 2;
    // A > B
    EXPECT_TRUE(HostFuncAVCmpQ.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ANum, ADen, BNum, BDen},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(1));

    ANum = 2;
    ADen = 1;
    BNum = 2;
    BDen = 1;
    // A == B
    EXPECT_TRUE(HostFuncAVCmpQ.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{ANum, ADen, BNum, BDen},
        Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(0));
  }

  FuncInst = AVUtilMod->findFuncExports("wasmedge_ffmpeg_avutil_av_reduce");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncAVReduce =
      dynamic_cast<WasmEdge::Host::WasmEdgeFFmpeg::AVUtil::AVReduce &>(
          FuncInst->getHostFunc());

  {
    int64_t ANum = 1;
    int64_t ADen = 2;
    int64_t Max = 3;
    EXPECT_TRUE(
        HostFuncAVReduce.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 NumPtr, DenPtr, ANum, ADen, Max},
                             Result));
    EXPECT_EQ(Result[0].get<int32_t>(), static_cast<int32_t>(1));
  }
}
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
