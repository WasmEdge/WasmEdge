#pragma once

#include "swscale_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace SWScale{

class SwsGetContext : public WasmEdgeFFmpegSWScale<SwsGetContext> {
public:
  SwsGetContext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t SwsCtxPtr,uint32_t SrcW,uint32_t SrcH,uint32_t SrcAvPixFormatId,uint32_t DesW,uint32_t DesH,uint32_t DesAvPixFormatId,int32_t Flags,uint32_t SrcFilterId, uint32_t DesFilterId);
};

class SwsFreeContext : public WasmEdgeFFmpegSWScale<SwsFreeContext> {
public:
  SwsFreeContext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t SwsCtxId);
};

class SwsScale : public WasmEdgeFFmpegSWScale<SwsScale> {
public:
  SwsScale(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t SwsCtxId,uint32_t InputFrameId,int32_t SrcSliceY,int32_t SrcSliceH,uint32_t OutputFrameId);
};

class SwsGetCachedContext : public WasmEdgeFFmpegSWScale<SwsGetCachedContext> {
public:
  SwsGetCachedContext(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t SwsCachedCtxPtr,uint32_t SwsCtxPtr,uint32_t SrcW,uint32_t SrcH,uint32_t SrcAvPixFormatId,uint32_t DesW,uint32_t DesH,uint32_t DesAvPixFormatId,int32_t Flags,uint32_t SrcFilterId, uint32_t DesFilterId);
};

class SwsIsSupportedInput : public WasmEdgeFFmpegSWScale<SwsIsSupportedInput> {
public:
  SwsIsSupportedInput(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t AvPixFormatId);
};

class SwsIsSupportedOutput : public WasmEdgeFFmpegSWScale<SwsIsSupportedOutput> {
public:
  SwsIsSupportedOutput(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t AvPixFormatId);
};

class SwsIsSupportedEndiannessConversion  : public WasmEdgeFFmpegSWScale<SwsIsSupportedEndiannessConversion> {
public:
  SwsIsSupportedEndiannessConversion(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegSWScale(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t AvPixFormatId);
};

} // namespace SWScale
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
