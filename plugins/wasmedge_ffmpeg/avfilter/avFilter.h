#pragma once

#include "avfilter_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFilter {

class AVFilterNameLength : public WasmEdgeFFmpegAVFilter<AVFilterNameLength> {
public:
  AVFilterNameLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId);
};

class AVFilterName : public WasmEdgeFFmpegAVFilter<AVFilterName> {
public:
  AVFilterName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId,
                       uint32_t NamePtr, uint32_t NameLen);
};

class AVFilterDescriptionLength
    : public WasmEdgeFFmpegAVFilter<AVFilterDescriptionLength> {
public:
  AVFilterDescriptionLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId);
};

class AVFilterDescription : public WasmEdgeFFmpegAVFilter<AVFilterDescription> {
public:
  AVFilterDescription(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId,
                       uint32_t DescPtr, uint32_t DescLen);
};

class AVFilterNbInputs : public WasmEdgeFFmpegAVFilter<AVFilterNbInputs> {
public:
  AVFilterNbInputs(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId);
};

class AVFilterNbOutputs : public WasmEdgeFFmpegAVFilter<AVFilterNbOutputs> {
public:
  AVFilterNbOutputs(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId);
};

class AVFilterFlags : public WasmEdgeFFmpegAVFilter<AVFilterFlags> {
public:
  AVFilterFlags(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId);
};

class AVFilterInOutSetName
    : public WasmEdgeFFmpegAVFilter<AVFilterInOutSetName> {
public:
  AVFilterInOutSetName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t InOutId,
                       uint32_t NamePtr, uint32_t NameLen);
};

class AVFilterInOutSetFilterCtx
    : public WasmEdgeFFmpegAVFilter<AVFilterInOutSetFilterCtx> {
public:
  AVFilterInOutSetFilterCtx(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t InOutId,
                       uint32_t FilterCtxId);
};

class AVFilterInOutSetPadIdx
    : public WasmEdgeFFmpegAVFilter<AVFilterInOutSetPadIdx> {
public:
  AVFilterInOutSetPadIdx(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t InOutId,
                       int32_t PadIdx);
};

class AVFilterInOutSetNext
    : public WasmEdgeFFmpegAVFilter<AVFilterInOutSetNext> {
public:
  AVFilterInOutSetNext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t InOutId,
                       uint32_t NextInOutId);
};

class AVFilterGetInputsFilterPad
    : public WasmEdgeFFmpegAVFilter<AVFilterGetInputsFilterPad> {
public:
  AVFilterGetInputsFilterPad(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId,
                       uint32_t FilterPadPtr);
};

class AVFilterGetOutputsFilterPad
    : public WasmEdgeFFmpegAVFilter<AVFilterGetOutputsFilterPad> {
public:
  AVFilterGetOutputsFilterPad(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFilter(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t FilterId,
                       uint32_t FilterPadPtr);
};

} // namespace AVFilter
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
