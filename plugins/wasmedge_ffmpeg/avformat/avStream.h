#pragma once

#include "avformat_base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class AVStreamId : public WasmEdgeFFmpegAVFormat<AVStreamId> {
public:
  AVStreamId(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamIndex : public WasmEdgeFFmpegAVFormat<AVStreamIndex> {
public:
  AVStreamIndex(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamCodecPar : public WasmEdgeFFmpegAVFormat<AVStreamCodecPar> {
public:
  AVStreamCodecPar(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx,
                       uint32_t CodecParameterPtr);
};

class AVStreamTimebase : public WasmEdgeFFmpegAVFormat<AVStreamTimebase> {
public:
  AVStreamTimebase(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t NumPtr,
                       uint32_t DenPtr, uint32_t AvFormatCtxId,
                       uint32_t StreamIdx);
};

class AVStreamSetTimebase : public WasmEdgeFFmpegAVFormat<AVStreamSetTimebase> {
public:
  AVStreamSetTimebase(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t Num,
                       uint32_t Den, uint32_t AvFormatCtxId,
                       uint32_t StreamIdx);
};

class AVStreamDuration : public WasmEdgeFFmpegAVFormat<AVStreamDuration> {
public:
  AVStreamDuration(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamStartTime : public WasmEdgeFFmpegAVFormat<AVStreamStartTime> {
public:
  AVStreamStartTime(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamNbFrames : public WasmEdgeFFmpegAVFormat<AVStreamNbFrames> {
public:
  AVStreamNbFrames(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamDisposition : public WasmEdgeFFmpegAVFormat<AVStreamDisposition> {
public:
  AVStreamDisposition(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamRFrameRate : public WasmEdgeFFmpegAVFormat<AVStreamRFrameRate> {
public:
  AVStreamRFrameRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t NumPtr,
                       uint32_t DenPtr, uint32_t AvFormatCtxId,
                       uint32_t StreamIdx);
};

class AVStreamSetRFrameRate
    : public WasmEdgeFFmpegAVFormat<AVStreamSetRFrameRate> {
public:
  AVStreamSetRFrameRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t Num,
                       int32_t Den, uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamAvgFrameRate
    : public WasmEdgeFFmpegAVFormat<AVStreamAvgFrameRate> {
public:
  AVStreamAvgFrameRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t NumPtr,
                       uint32_t DenPtr, uint32_t AvFormatCtxId,
                       uint32_t StreamIdx);
};

class AVStreamSetAvgFrameRate
    : public WasmEdgeFFmpegAVFormat<AVStreamSetAvgFrameRate> {
public:
  AVStreamSetAvgFrameRate(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t Num,
                       int32_t Den, uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamMetadata : public WasmEdgeFFmpegAVFormat<AVStreamMetadata> {
public:
  AVStreamMetadata(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx,
                       uint32_t DictPtr);
};

class AVStreamSetMetadata : public WasmEdgeFFmpegAVFormat<AVStreamSetMetadata> {
public:
  AVStreamSetMetadata(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx,
                       uint32_t DictId);
};

class AVStreamDiscard : public WasmEdgeFFmpegAVFormat<AVStreamDiscard> {
public:
  AVStreamDiscard(std::shared_ptr<WasmEdgeFFmpegEnv> HostEnv)
      : WasmEdgeFFmpegAVFormat(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
