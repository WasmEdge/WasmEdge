// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class AVStreamId : public HostFunction<AVStreamId> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamIndex : public HostFunction<AVStreamIndex> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamCodecPar : public HostFunction<AVStreamCodecPar> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx,
                       uint32_t CodecParameterPtr);
};

class AVStreamTimebase : public HostFunction<AVStreamTimebase> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t NumPtr,
                       uint32_t DenPtr, uint32_t AvFormatCtxId,
                       uint32_t StreamIdx);
};

class AVStreamSetTimebase : public HostFunction<AVStreamSetTimebase> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t Num,
                       uint32_t Den, uint32_t AvFormatCtxId,
                       uint32_t StreamIdx);
};

class AVStreamDuration : public HostFunction<AVStreamDuration> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamStartTime : public HostFunction<AVStreamStartTime> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamNbFrames : public HostFunction<AVStreamNbFrames> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamDisposition : public HostFunction<AVStreamDisposition> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamRFrameRate : public HostFunction<AVStreamRFrameRate> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t NumPtr,
                       uint32_t DenPtr, uint32_t AvFormatCtxId,
                       uint32_t StreamIdx);
};

class AVStreamSetRFrameRate : public HostFunction<AVStreamSetRFrameRate> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t Num,
                       int32_t Den, uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamAvgFrameRate : public HostFunction<AVStreamAvgFrameRate> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t NumPtr,
                       uint32_t DenPtr, uint32_t AvFormatCtxId,
                       uint32_t StreamIdx);
};

class AVStreamSetAvgFrameRate : public HostFunction<AVStreamSetAvgFrameRate> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t Num,
                       int32_t Den, uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

class AVStreamMetadata : public HostFunction<AVStreamMetadata> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx,
                       uint32_t DictPtr);
};

class AVStreamSetMetadata : public HostFunction<AVStreamSetMetadata> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx,
                       uint32_t DictId);
};

class AVStreamDiscard : public HostFunction<AVStreamDiscard> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t StreamIdx);
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
