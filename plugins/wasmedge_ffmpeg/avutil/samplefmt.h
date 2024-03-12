#pragma once
#include "avutil_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

class AVGetPlanarSampleFmt : public WasmEdgeFFmpegAVUtil<AVGetPlanarSampleFmt> {
public:
  AVGetPlanarSampleFmt(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t SampleFormatId);
};

class AVGetPackedSampleFmt : public WasmEdgeFFmpegAVUtil<AVGetPackedSampleFmt> {
public:
  AVGetPackedSampleFmt(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t SampleFormatId);
};

class AVSampleFmtIsPlanar : public WasmEdgeFFmpegAVUtil<AVSampleFmtIsPlanar> {
public:
  AVSampleFmtIsPlanar(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t SampleFormatId);
};

class AVGetBytesPerSample : public WasmEdgeFFmpegAVUtil<AVGetBytesPerSample> {
public:
  AVGetBytesPerSample(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SampleFormatId);
};

class AVGetSampleFmt : public WasmEdgeFFmpegAVUtil<AVGetSampleFmt> {
public:
  AVGetSampleFmt(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t Str,
                       uint32_t StrLen);
};

class AVSamplesGetBufferSize
    : public WasmEdgeFFmpegAVUtil<AVSamplesGetBufferSize> {
public:
  AVSamplesGetBufferSize(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t NbChannels,
                       int32_t NbSamples, uint32_t SampleFormatId,
                       int32_t Align);
};

class AVSamplesAllocArrayAndSamples
    : public WasmEdgeFFmpegAVUtil<AVSamplesAllocArrayAndSamples> {
public:
  AVSamplesAllocArrayAndSamples(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t BufferPtr,
                       uint32_t LinesizePtr, int32_t NbChannels,
                       int32_t NbSamples, uint32_t SampleFmtId, int32_t Align);
};

class AVGetSampleFmtNameLength
    : public WasmEdgeFFmpegAVUtil<AVGetSampleFmtNameLength> {
public:
  AVGetSampleFmtNameLength(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SampleFmtId);
};

class AVGetSampleFmtName : public WasmEdgeFFmpegAVUtil<AVGetSampleFmtName> {
public:
  AVGetSampleFmtName(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SampleFmtId,
                       uint32_t SampleFmtNamePtr, uint32_t SampleFmtNameLen);
};

class AVGetSampleFmtMask : public WasmEdgeFFmpegAVUtil<AVGetSampleFmtMask> {
public:
  AVGetSampleFmtMask(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t SampleFmtId);
};

class AVFreep : public WasmEdgeFFmpegAVUtil<AVFreep> {
public:
  AVFreep(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t BufferId);
};

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
