// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avRational.h"

extern "C" {
#include "libavutil/rational.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

Expect<int32_t> AVAddQ::body(const Runtime::CallingFrame &Frame, int32_t ANum,
                             int32_t ADen, int32_t BNum, int32_t BDen,
                             uint32_t CNumPtr, uint32_t CDenPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(CNum, MemInst, int32_t, CNumPtr,
                "Failed to access Numerator Ptr for AVRational"sv);
  MEM_PTR_CHECK(CDen, MemInst, int32_t, CDenPtr,
                "Failed to access Denominator Ptr for AVRational"sv);

  AVRational const A = av_make_q(ANum, ADen);
  AVRational const B = av_make_q(BNum, BDen);

  AVRational const C = av_add_q(A, B);
  *CNum = C.num;
  *CDen = C.den;

  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVSubQ::body(const Runtime::CallingFrame &Frame, int32_t ANum,
                             int32_t ADen, int32_t BNum, int32_t BDen,
                             uint32_t CNumPtr, uint32_t CDenPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(CNum, MemInst, int32_t, CNumPtr,
                "Failed to access Numerator Ptr for AVRational"sv);
  MEM_PTR_CHECK(CDen, MemInst, int32_t, CDenPtr,
                "Failed to access Denominator Ptr for AVRational"sv);

  AVRational const A = av_make_q(ANum, ADen);
  AVRational const B = av_make_q(BNum, BDen);

  AVRational const C = av_sub_q(A, B);
  *CNum = C.num;
  *CDen = C.den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVMulQ::body(const Runtime::CallingFrame &Frame, int32_t ANum,
                             int32_t ADen, int32_t BNum, int32_t BDen,
                             uint32_t CNumPtr, uint32_t CDenPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(CNum, MemInst, int32_t, CNumPtr,
                "Failed to access Numerator Ptr for AVRational"sv);
  MEM_PTR_CHECK(CDen, MemInst, int32_t, CDenPtr,
                "Failed to access Denominator Ptr for AVRational"sv);

  AVRational const A = av_make_q(ANum, ADen);
  AVRational const B = av_make_q(BNum, BDen);

  AVRational const C = av_mul_q(A, B);
  *CNum = C.num;
  *CDen = C.den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVDivQ::body(const Runtime::CallingFrame &Frame, int32_t ANum,
                             int32_t ADen, int32_t BNum, int32_t BDen,
                             uint32_t CNumPtr, uint32_t CDenPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(CNum, MemInst, int32_t, CNumPtr,
                "Failed to access Numerator Ptr for AVRational"sv);
  MEM_PTR_CHECK(CDen, MemInst, int32_t, CDenPtr,
                "Failed to access Denominator Ptr for AVRational"sv);

  AVRational const A = av_make_q(ANum, ADen);
  AVRational const B = av_make_q(BNum, BDen);

  AVRational const C = av_div_q(A, B);
  *CNum = C.num;
  *CDen = C.den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVCmpQ::body(const Runtime::CallingFrame &, int32_t ANum,
                             int32_t ADen, int32_t BNum, int32_t BDen) {
  AVRational const A = av_make_q(ANum, ADen);
  AVRational const B = av_make_q(BNum, BDen);
  return av_cmp_q(A, B);
}

Expect<int32_t> AVNearerQ::body(const Runtime::CallingFrame &, int32_t ANum,
                                int32_t ADen, int32_t BNum, int32_t BDen,
                                int32_t CNum, int32_t CDen) {
  AVRational const A = av_make_q(ANum, ADen);
  AVRational const B = av_make_q(BNum, BDen);
  AVRational const C = av_make_q(CNum, CDen);

  return av_nearer_q(A, B, C);
}

Expect<double_t> AVQ2d::body(const Runtime::CallingFrame &, int32_t ANum,
                             int32_t ADen) {
  AVRational const A = av_make_q(ANum, ADen);
  return av_q2d(A);
}

Expect<int32_t> AVD2Q::body(const Runtime::CallingFrame &Frame, double_t D,
                            int32_t Max, uint32_t ANumPtr, uint32_t ADenPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(ANum, MemInst, int32_t, ANumPtr,
                "Failed to access Numerator Ptr for AVRational"sv);
  MEM_PTR_CHECK(ADen, MemInst, int32_t, ADenPtr,
                "Failed to access Denominator Ptr for AVRational"sv);

  AVRational const A = av_d2q(D, Max);
  *ANum = A.num;
  *ADen = A.den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint32_t> AVQ2IntFloat::body(const Runtime::CallingFrame &, int32_t ANum,
                                    int32_t ADen) {
  AVRational const A = av_make_q(ANum, ADen);
  return av_q2intfloat(A);
}

Expect<int32_t> AVInvQ::body(const Runtime::CallingFrame &Frame, int32_t ANum,
                             int32_t ADen, uint32_t BNumPtr, uint32_t BDenPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(BNum, MemInst, int32_t, BNumPtr,
                "Failed to access Numerator Ptr for AVRational"sv);
  MEM_PTR_CHECK(BDen, MemInst, int32_t, BDenPtr,
                "Failed to access Denominator Ptr for AVRational"sv);

  AVRational const A = av_make_q(ANum, ADen);
  AVRational const B = av_inv_q(A);

  *BNum = B.num;
  *BDen = B.den;
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVReduce::body(const Runtime::CallingFrame &Frame,
                               uint32_t ANumPtr, uint32_t ADenPtr, int64_t BNum,
                               int64_t BDen, int64_t Max) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(ANum, MemInst, int32_t, ANumPtr,
                "Failed to access Numerator Ptr for AVRational"sv);
  MEM_PTR_CHECK(ADen, MemInst, int32_t, ADenPtr,
                "Failed to access Denominator Ptr for AVRational"sv);
  return av_reduce(ANum, ADen, BNum, BDen, Max);
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
