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
                       uint32_t AVFilterContextId, uint32_t AVFrameId);
};

class AVBufferSinkGetSamples
    : public WasmEdgeFFmpegAVFilter<AVBufferSinkGetSamples> {
public:
  AVBufferSinkGetSamples(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVFilterContextId, uint32_t AVFrameId,
                       int32_t Samples);
};

class AvBufferSinkSetFrameSize
    : public WasmEdgeFFmpegAVFilter<AvBufferSinkSetFrameSize> {
public:
  AvBufferSinkSetFrameSize(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVFilterContextId, int32_t Value);
};

class AVBufferSrcGetNbFailedRequests
    : public WasmEdgeFFmpegAVFilter<AVBufferSrcGetNbFailedRequests> {
public:
  AVBufferSrcGetNbFailedRequests(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVFilterContextId);
};

class AVBufferSrcAddFrame : public WasmEdgeFFmpegAVFilter<AVBufferSrcAddFrame> {
public:
  AVBufferSrcAddFrame(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVFilterContextId, uint32_t AVFrameId);
};

class AVBufferSrcClose : public WasmEdgeFFmpegAVFilter<AVBufferSrcClose> {
public:
  AVBufferSrcClose(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVFilterContextId, int64_t Pts, uint32_t Flags);
};

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
