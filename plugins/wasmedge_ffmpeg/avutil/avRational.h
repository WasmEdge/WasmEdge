// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVAddQ : public HostFunction<AVAddQ> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                       int32_t ADen, int32_t BNum, int32_t BDen,
                       uint32_t CNumPtr, uint32_t CDenPtr);
};

class AVSubQ : public HostFunction<AVSubQ> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                       int32_t ADen, int32_t BNum, int32_t BDen,
                       uint32_t CNumPtr, uint32_t CDenPtr);
};

class AVMulQ : public HostFunction<AVMulQ> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                       int32_t ADen, int32_t BNum, int32_t BDen,
                       uint32_t CNumPtr, uint32_t CDenPtr);
};

class AVDivQ : public HostFunction<AVDivQ> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                       int32_t ADen, int32_t BNum, int32_t BDen,
                       uint32_t CNumPtr, uint32_t CDenPtr);
};

class AVCmpQ : public HostFunction<AVCmpQ> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                       int32_t ADen, int32_t BNum, int32_t BDen);
};

class AVNearerQ : public HostFunction<AVNearerQ> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                       int32_t ADen, int32_t BNum, int32_t BDen, int32_t CNum,
                       int32_t CDen);
};

class AVQ2d : public HostFunction<AVQ2d> {
public:
  using HostFunction::HostFunction;
  Expect<double_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                        int32_t ADen);
};

class AVD2Q : public HostFunction<AVD2Q> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, double_t D,
                       int32_t Max, uint32_t ANumPtr, uint32_t ADenPtr);
};

class AVQ2IntFloat : public HostFunction<AVQ2IntFloat> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                        int32_t ADen);
};

class AVInvQ : public HostFunction<AVInvQ> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t ANum,
                       int32_t ADen, uint32_t BNumPtr, uint32_t BDenPtr);
};

class AVReduce : public HostFunction<AVReduce> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ANumPtr,
                       uint32_t ADenPtr, int64_t BNum, int64_t BDen,
                       int64_t Max);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
