#pragma once

#include "avfilter_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter {

class AVBufferSinkGetFrame
    : public WasmEdgeFFmpegAVFilter<AVBufferSinkGetFrame> {
public:
  AVBufferSinkGetFrame(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterContextId, uint32_t FrameId);
};

class AVBufferSinkGetSamples
    : public WasmEdgeFFmpegAVFilter<AVBufferSinkGetSamples> {
public:
  AVBufferSinkGetSamples(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterContextId, uint32_t FrameId,
                       int32_t Samples);
};

class AvBufferSinkSetFrameSize
    : public WasmEdgeFFmpegAVFilter<AvBufferSinkSetFrameSize> {
public:
  AvBufferSinkSetFrameSize(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterContextId, int32_t Value);
};

class AVBufferSrcGetNbFailedRequests
    : public WasmEdgeFFmpegAVFilter<AVBufferSrcGetNbFailedRequests> {
public:
  AVBufferSrcGetNbFailedRequests(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterContextId);
};

class AVBufferSrcAddFrame : public WasmEdgeFFmpegAVFilter<AVBufferSrcAddFrame> {
public:
  AVBufferSrcAddFrame(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterContextId, uint32_t FrameId);
};

class AVBufferSrcClose : public WasmEdgeFFmpegAVFilter<AVBufferSrcClose> {
public:
  AVBufferSrcClose(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t FilterContextId, int64_t Pts, uint32_t Flags);
};

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
