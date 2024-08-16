// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class AVFormatCtxIFormat : public HostFunction<AVFormatCtxIFormat> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t AvInputFormatPtr);
};

class AVFormatCtxOFormat : public HostFunction<AVFormatCtxOFormat> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t AvOutputFormatPtr);
};

class AVFormatCtxProbeScore : public HostFunction<AVFormatCtxProbeScore> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId);
};

class AVFormatCtxNbStreams : public HostFunction<AVFormatCtxNbStreams> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvFormatCtxId);
};

class AVFormatCtxBitRate : public HostFunction<AVFormatCtxBitRate> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId);
};

class AVFormatCtxDuration : public HostFunction<AVFormatCtxDuration> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId);
};

class AVFormatCtxNbChapters : public HostFunction<AVFormatCtxNbChapters> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AvFormatCtxId);
};

class AVFormatCtxSetNbChapters : public HostFunction<AVFormatCtxSetNbChapters> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t NbChapters);
};

class AVFormatCtxMetadata : public HostFunction<AVFormatCtxMetadata> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t DictPtr);
};

class AVFormatCtxSetMetadata : public HostFunction<AVFormatCtxSetMetadata> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t DictId);
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
