#pragma once

#include "avfilter_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter{

class AVFilterGraphAlloc : public WasmEdgeFFmpegAVFilter<AVFilterGraphAlloc> {
public:
  AVFilterGraphAlloc(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t AVFilterGraphPtr);
};

class AVFilterGraphConfig : public WasmEdgeFFmpegAVFilter<AVFilterGraphConfig> {
public:
  AVFilterGraphConfig(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t AVFilterGraphId);
};

class AVFilterGraphFree : public WasmEdgeFFmpegAVFilter<AVFilterGraphFree> {
public:
  AVFilterGraphFree (std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t AVFilterGraphId);
};

class AVFilterGraphGetFilter : public WasmEdgeFFmpegAVFilter<AVFilterGraphGetFilter> {
public:
  AVFilterGraphGetFilter(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t AVFilterCtxPtr,uint32_t AVFilterGraphId,uint32_t NamePtr, uint32_t NameSize);
};

class AVFilterGraphParsePtr : public WasmEdgeFFmpegAVFilter<AVFilterGraphParsePtr> {
public:
  AVFilterGraphParsePtr(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t AVFilterGraphId,uint32_t FiltersString,uint32_t FiltersSize,uint32_t InputsId,uint32_t OutputsId);
};

class AVFilterInOutFree : public WasmEdgeFFmpegAVFilter<AVFilterInOutFree> {
public:
  AVFilterInOutFree(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t InoutId);
};

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
