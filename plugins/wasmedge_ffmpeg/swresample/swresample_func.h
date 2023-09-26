#pragma once

#include "runtime/callingframe.h"
#include "swresample_base.h"
#include "ffmpeg_env.h"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWResample {

class SWResampleVersion : public WasmEdgeFFmpegSWResample<SWResampleVersion> {
public:
  SWResampleVersion(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWResample(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class SWRGetDelay: public WasmEdgeFFmpegSWResample<SWRGetDelay> {
public:
  SWRGetDelay(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWResample(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,uint32_t SWRContextId,int64_t Base);
};

class SWRInit: public WasmEdgeFFmpegSWResample<SWRInit> {
public:
  SWRInit(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWResample(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t SWRContextId);
};

class SWRAllocSetOpts : public WasmEdgeFFmpegSWResample<SWRAllocSetOpts> {
public:
  SWRAllocSetOpts(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWResample(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t SwrCtxPtr,uint32_t SWRContextId,int64_t OutChLayout,uint32_t OutSampleFmtId,int32_t OutSampleRate,int64_t InChLayout, uint32_t InSampleFmtId,int32_t InSampleRate,int32_t LogOffset);
};

class AVOptSetDict : public WasmEdgeFFmpegSWResample<AVOptSetDict> {
public:
  AVOptSetDict(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWResample(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t SWRContextId,uint32_t DictId);
};

class SWRConvertFrame: public WasmEdgeFFmpegSWResample<SWRConvertFrame> {
public:
  SWRConvertFrame(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWResample(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t SWRContextId,uint32_t FrameOutputId, uint32_t FrameInputId);
};


class SWRFree: public WasmEdgeFFmpegSWResample<SWRFree> {
public:
  SWRFree(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWResample(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t SWRContextId);
};

} // namespace SWResample
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge