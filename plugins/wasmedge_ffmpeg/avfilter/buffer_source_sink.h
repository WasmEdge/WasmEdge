// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter {

class AVBufferSinkGetFrame : public HostFunction<AVBufferSinkGetFrame> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterContextId, uint32_t FrameId);
};

class AVBufferSinkGetSamples : public HostFunction<AVBufferSinkGetSamples> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterContextId, uint32_t FrameId,
                       int32_t Samples);
};

class AvBufferSinkSetFrameSize : public HostFunction<AvBufferSinkSetFrameSize> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterContextId, int32_t Value);
};

class AVBufferSrcGetNbFailedRequests
    : public HostFunction<AVBufferSrcGetNbFailedRequests> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterContextId);
};

class AVBufferSrcAddFrame : public HostFunction<AVBufferSrcAddFrame> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterContextId, uint32_t FrameId);
};

class AVBufferSrcClose : public HostFunction<AVBufferSrcClose> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterContextId, int64_t Pts, uint32_t Flags);
};

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
