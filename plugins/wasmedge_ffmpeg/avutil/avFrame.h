#pragma once
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include "avutil_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil{

class AVFrameAlloc : public WasmEdgeFFmpegAVUtil<AVFrameAlloc> {
public:
  AVFrameAlloc(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t FramePtr);
};

class AVFrameFree : public WasmEdgeFFmpegAVUtil<AVFrameFree> {
public:
  AVFrameFree(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId);
};

class AVFrameWidth : public WasmEdgeFFmpegAVUtil<AVFrameWidth> {
public:
  AVFrameWidth(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId);
};

class AVFrameHeight : public WasmEdgeFFmpegAVUtil<AVFrameHeight> {
public:
  AVFrameHeight(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId);
};

class AVFrameSetWidth : public WasmEdgeFFmpegAVUtil<AVFrameSetWidth> {
public:
  AVFrameSetWidth(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame,uint32_t FrameId,uint32_t Width);
};

class AVFrameSetHeight : public WasmEdgeFFmpegAVUtil<AVFrameSetHeight> {
public:
  AVFrameSetHeight(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame,uint32_t FrameId,uint32_t Height);
};

class AVFrameVideoFormat : public WasmEdgeFFmpegAVUtil<AVFrameVideoFormat> {
public:
  AVFrameVideoFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId);
};

class AVFrameSetVideoFormat : public WasmEdgeFFmpegAVUtil<AVFrameSetVideoFormat> {
public:
  AVFrameSetVideoFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId,uint32_t AvPixFormatId);
};

class AVFrameIsNull : public WasmEdgeFFmpegAVUtil<AVFrameIsNull> {
public:
  AVFrameIsNull(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId);
};


class AVFrameLinesize : public WasmEdgeFFmpegAVUtil<AVFrameLinesize> {
public:
  AVFrameLinesize(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId,uint32_t Idx);
};

class AVFrameData : public WasmEdgeFFmpegAVUtil<AVFrameData> {
public:
  AVFrameData(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId,uint32_t FrameBufPtr,uint32_t FrameBufLen);
};

class AVFrameGetBuffer : public WasmEdgeFFmpegAVUtil<AVFrameGetBuffer> {
public:
  AVFrameGetBuffer(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId,int32_t Align);
};


class AVFrameAudioFormat : public WasmEdgeFFmpegAVUtil<AVFrameAudioFormat> {
public:
  AVFrameAudioFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId);
};

class AVFrameSetAudioFormat : public WasmEdgeFFmpegAVUtil<AVFrameSetAudioFormat> {
public:
  AVFrameSetAudioFormat(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId,uint32_t SampleFormatId);
};

class AVFrameSetChannelLayout : public WasmEdgeFFmpegAVUtil<AVFrameSetChannelLayout> {
public:
  AVFrameSetChannelLayout(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId,uint64_t ChannelLayoutID);
};

class AVFrameSetNbSamples : public WasmEdgeFFmpegAVUtil<AVFrameSetNbSamples> {
public:
  AVFrameSetNbSamples(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId,int32_t Samples);
};

class AVFrameNbSamples : public WasmEdgeFFmpegAVUtil<AVFrameNbSamples> {
public:
  AVFrameNbSamples(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId);
};

class AVFrameSampleRate : public WasmEdgeFFmpegAVUtil<AVFrameSampleRate> {
public:
  AVFrameSampleRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId);
};

class AVFrameChannels : public WasmEdgeFFmpegAVUtil<AVFrameChannels> {
public:
  AVFrameChannels(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVUtil(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,uint32_t FrameId);
};


} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
